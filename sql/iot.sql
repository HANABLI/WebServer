CREATE EXTENSION IF NOT EXISTS pgcrypto;
CREATE EXTENSION IF NOT EXISTS citext;
CREATE SCHEMA IF NOT EXISTS iot;
SET search_path = iot, public;

DO $$ BEGIN
  CREATE TYPE tenant_role AS ENUM ('admin','operator','viewer');
EXCEPTION WHEN duplicate_object THEN NULL; END $$;
DO $$ BEGIN
  CREATE TYPE device_protocol AS ENUM ('mqtt','modbus-tcp','opcua');
EXCEPTION WHEN duplicate_object THEN NULL; END $$;

DO $$ BEGIN
  CREATE TYPE topic_direction AS ENUM ('sub','pub','pubsub');
EXCEPTION WHEN duplicate_object THEN NULL; END $$;

DO $$ BEGIN
  CREATE TYPE severity_level AS ENUM ('info','low','medium','high','critical');
EXCEPTION WHEN duplicate_object THEN NULL; END $$;

DO $$ BEGIN
  CREATE TYPE event_source AS ENUM ('iot','vision','ai','system','user');
EXCEPTION WHEN duplicate_object THEN NULL; END $$;

-- Schema iot déjà existant chez toi

CREATE TABLE IF NOT EXISTS iot.tenants (
  id uuid PRIMARY KEY ,
  slug text NOT NULL UNIQUE,
  name text NOT NULL,
  created_at timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS iot.users (
  id uuid PRIMARY KEY ,
  tenant_id uuid NOT NULL REFERENCES iot.tenants(id) ON DELETE CASCADE,
  user_name text NOT NULL,
  email text,
  disabled boolean NOT NULL DEFAULT false,

  password_hash text NOT NULL,        -- libsodium crypto_pwhash_str()
  mfa_enabled boolean NOT NULL DEFAULT false,
  totp_secret_b32 text,               -- base32 (chiffrable plus tard)
  totp_digits int NOT NULL DEFAULT 6,
  totp_period int NOT NULL DEFAULT 30,

  role text NOT NULL DEFAULT 'viewer', -- admin|operator|viewer (tenant-wide role)
  site_roles jsonb NOT NULL DEFAULT '{}'::jsonb,
  created_at timestamptz NOT NULL DEFAULT now(),
  updated_at timestamptz NOT NULL DEFAULT now()
);

CREATE UNIQUE INDEX IF NOT EXISTS ux_users_tenant_user_name ON iot.users(tenant_id, user_name);
CREATE INDEX IF NOT EXISTS ix_users_tenant_role ON iot.users(tenant_id, role);

-- ============================================================
-- Invitations
-- ============================================================
CREATE TABLE IF NOT EXISTS iot.invitations (
  id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  tenant_id uuid NOT NULL REFERENCES iot.tenants(id) ON DELETE CASCADE,
  email citext NOT NULL,
  role text NOT NULL DEFAULT 'viewer', -- admin/operator/viewer
  site_id uuid REFERENCES iot.sites(id) ON DELETE SET NULL,
  token_hash bytea NOT NULL,
  created_at timestamptz NOT NULL DEFAULT now(),
  expires_at timestamptz NOT NULL,
  accepted_at timestamptz,
  accepted_user_id uuid REFERENCES iot.users(id) ON DELETE SET NULL,
  created_by uuid REFERENCES iot.users(id) ON DELETE SET NULL
);

CREATE INDEX IF NOT EXISTS ix_invite_token_hash ON iot.invitations(token_hash);
CREATE INDEX IF NOT EXISTS ix_invite_tenant_email ON iot.invitations(tenant_id, email);
CREATE INDEX IF NOT EXISTS ix_invite_exp ON iot.invitations(expires_at);

-- ============================================================
-- Email outbox (SMTP worker)
-- ============================================================
CREATE TABLE IF NOT EXISTS iot.outbox_emails (
  id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  ts timestamptz NOT NULL DEFAULT now(),
  status text NOT NULL DEFAULT 'pending', -- pending/sent/failed
  to_email citext NOT NULL,
  subject text NOT NULL,
  body text NOT NULL,
  tries int NOT NULL DEFAULT 0,
  last_error text
);

CREATE INDEX IF NOT EXISTS ix_outbox_status_ts ON iot.outbox_emails(status, ts);

-- -------------------------
-- Refresh tokens
-- -------------------------
CREATE TABLE IF NOT EXISTS iot.refresh_tokens (
  id uuid PRIMARY KEY DEFAULT gen_random_uuid(),
  tenant_id uuid NOT NULL REFERENCES iot.tenants(id) ON DELETE CASCADE,
  user_id uuid NOT NULL REFERENCES iot.users(id) ON DELETE CASCADE,
  token_hash bytea NOT NULL,        -- hash(refresh_token)
  created_at timestamptz NOT NULL DEFAULT now(),
  expires_at timestamptz NOT NULL,
  revoked_at timestamptz,
  user_agent text,
  ip text
);

CREATE INDEX IF NOT EXISTS ix_refresh_tokens_user ON iot.refresh_tokens(tenant_id, user_id, created_at DESC);
CREATE INDEX IF NOT EXISTS ix_refresh_tokens_hash ON iot.refresh_tokens(token_hash);
CREATE INDEX IF NOT EXISTS ix_refresh_tokens_exp ON iot.refresh_tokens(expires_at);

-- -------------------------
-- updated_at trigger
-- -------------------------
CREATE OR REPLACE FUNCTION iot.set_updated_at() RETURNS trigger LANGUAGE plpgsql AS $$
BEGIN NEW.updated_at = now(); RETURN NEW; END $$;

DO $$ BEGIN
  CREATE TRIGGER trg_users_updated BEFORE UPDATE ON iot.users
  FOR EACH ROW EXECUTE FUNCTION iot.set_updated_at();
EXCEPTION WHEN duplicate_object THEN NULL; END $$;

-- ============================================================
-- 2) Sites / Zones
-- ============================================================
CREATE TABLE IF NOT EXISTS sites (
  id uuid PRIMARY KEY ,
  name citext NOT NULL,
  description text,
  timezone text NOT NULL DEFAULT 'Europe/Paris',
  created_at timestamptz NOT NULL DEFAULT now(),
  updated_at timestamptz NOT NULL DEFAULT now()
);
CREATE UNIQUE INDEX IF NOT EXISTS ux_sites_name ON sites(name);

CREATE TABLE IF NOT EXISTS zones (
  id uuid PRIMARY KEY ,
  site_id uuid NOT NULL REFERENCES sites(id) ON DELETE CASCADE,
  name citext NOT NULL,
  description text,
  kind text,
  geojson jsonb NOT NULL DEFAULT '{}'::jsonb,
  created_at timestamptz NOT NULL DEFAULT now(),
  updated_at timestamptz NOT NULL DEFAULT now()
);
CREATE UNIQUE INDEX IF NOT EXISTS ux_zones_site_name ON zones(site_id, name);
CREATE INDEX IF NOT EXISTS ix_zones_site ON zones(site_id);

-- ============================================================
-- 3) Servers (par type) - aligné ServerRepo.hpp
-- ============================================================
CREATE TABLE IF NOT EXISTS mqtt_servers (
  id uuid PRIMARY KEY ,
  name citext NOT NULL,
  enabled boolean NOT NULL DEFAULT true,
  host text NOT NULL,
  port int NOT NULL DEFAULT 1883,
  useTLS boolean NOT NULL DEFAULT false,

  cleanSession boolean NOT NULL DEFAULT true,
  willRetain boolean NOT NULL DEFAULT false,
  willTopic text,
  willPayload text,
  qos int NOT NULL DEFAULT 1,
  keepAlive int NOT NULL DEFAULT 60,

  created_at timestamptz NOT NULL DEFAULT now(),
  updated_at timestamptz NOT NULL DEFAULT now()
);
CREATE UNIQUE INDEX IF NOT EXISTS ux_mqtt_servers_name ON mqtt_servers(name);

CREATE TABLE IF NOT EXISTS mqtt_credentials (
  server_id uuid PRIMARY KEY REFERENCES mqtt_servers(id) ON DELETE CASCADE,
  user_name text,
  password text, -- (ou bytea chiffré si tu veux)
  updated_at timestamptz NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS modbus_servers (
  id uuid PRIMARY KEY ,
  name citext NOT NULL,
  enabled boolean NOT NULL DEFAULT true,
  host text NOT NULL,
  port int NOT NULL DEFAULT 502,
  created_at timestamptz NOT NULL DEFAULT now(),
  updated_at timestamptz NOT NULL DEFAULT now()
);
CREATE UNIQUE INDEX IF NOT EXISTS ux_modbus_servers_name ON modbus_servers(name);

CREATE TABLE IF NOT EXISTS opcua_servers (
  id uuid PRIMARY KEY ,
  name citext NOT NULL,
  enabled boolean NOT NULL DEFAULT true,
  endpoint_url text NOT NULL,
  created_at timestamptz NOT NULL DEFAULT now(),
  updated_at timestamptz NOT NULL DEFAULT now()
);
CREATE UNIQUE INDEX IF NOT EXISTS ux_opcua_servers_name ON opcua_servers(name);

-- ============================================================
-- 4) IoT Devices
-- ============================================================
CREATE TABLE IF NOT EXISTS devices (
  id uuid PRIMARY KEY ,
  site_id uuid NOT NULL REFERENCES sites(id) ON DELETE CASCADE,
  zone_id uuid REFERENCES zones(id) ON DELETE SET NULL,

  name citext NOT NULL,
  enabled boolean NOT NULL DEFAULT true,
  protocol device_protocol NOT NULL DEFAULT 'mqtt',

  -- server binding (id de mqtt_servers/modbus_servers/opcua_servers)
  server_id uuid,

  external_id text,
  metadata jsonb NOT NULL DEFAULT '{}'::jsonb,

  created_at timestamptz NOT NULL DEFAULT now(),
  updated_at timestamptz NOT NULL DEFAULT now()
);
CREATE UNIQUE INDEX IF NOT EXISTS ux_devices_site_name ON devices(site_id, name);
CREATE INDEX IF NOT EXISTS ix_devices_site_zone ON devices(site_id, zone_id);
CREATE INDEX IF NOT EXISTS ix_devices_protocol ON devices(protocol);
CREATE INDEX IF NOT EXISTS ix_devices_enabled ON devices(enabled);
CREATE INDEX IF NOT EXISTS ix_devices_server ON devices(server_id);

-- ============================================================
-- 5) Device Topics (auto-subscribe)
-- ============================================================
CREATE TABLE IF NOT EXISTS device_topics (
  id uuid PRIMARY KEY ,
  device_id uuid NOT NULL REFERENCES devices(id) ON DELETE CASCADE,

  topic text NOT NULL,
  direction topic_direction NOT NULL DEFAULT 'sub',
  qos int NOT NULL DEFAULT 1,
  retain boolean NOT NULL DEFAULT false,
  enabled boolean NOT NULL DEFAULT true,

  role text,              -- ex: telemetry / ack / command / state
  parse_hint text,        -- ex: json / plain / protobuf etc
  metadata jsonb NOT NULL DEFAULT '{}'::jsonb,

  created_at timestamptz NOT NULL DEFAULT now(),
  updated_at timestamptz NOT NULL DEFAULT now()
);
CREATE INDEX IF NOT EXISTS ix_topics_device ON device_topics(device_id);
CREATE INDEX IF NOT EXISTS ix_topics_enabled ON device_topics(enabled);
CREATE INDEX IF NOT EXISTS ix_topics_topic ON device_topics(topic);

-- ============================================================
-- 7) Commands pipeline
-- ============================================================
DO $$ BEGIN
  CREATE TYPE command_status AS ENUM ('pending','sent','acked','failed','cancelled');
EXCEPTION WHEN duplicate_object THEN NULL; END $$;

CREATE TABLE IF NOT EXISTS device_commands (
  id uuid PRIMARY KEY ,
  device_id uuid NOT NULL REFERENCES devices(id) ON DELETE CASCADE,

  created_at timestamptz NOT NULL DEFAULT now(),
  sent_at timestamptz,
  ack_at timestamptz,

  command text NOT NULL,                 -- ex: "open_valve"
  params jsonb NOT NULL DEFAULT '{}'::jsonb,

  status command_status NOT NULL DEFAULT 'pending',
  qos smallint NOT NULL DEFAULT 1,
  retain boolean NOT NULL DEFAULT false,
  topic_override text,                   -- optionnel si tu veux forcer un topic

  attempts int NOT NULL DEFAULT 0,
  last_error text
);

CREATE INDEX IF NOT EXISTS ix_cmd_device_created ON device_commands(device_id, created_at DESC);
CREATE INDEX IF NOT EXISTS ix_cmd_status_created ON device_commands(status, created_at DESC);

-- ============================================================
-- 8) Events (audit / notifications)
-- ============================================================
CREATE TABLE IF NOT EXISTS events (
  id uuid PRIMARY KEY ,
  ts timestamptz NOT NULL DEFAULT now(),
  source event_source NOT NULL DEFAULT 'iot',
  site_id uuid REFERENCES sites(id) ON DELETE SET NULL,
  zone_id uuid REFERENCES zones(id) ON DELETE SET NULL,
  device_id uuid REFERENCES devices(id) ON DELETE SET NULL,
  type text NOT NULL,
  severity severity_level NOT NULL DEFAULT 'info',
  payload jsonb NOT NULL DEFAULT '{}'::jsonb
);
CREATE INDEX IF NOT EXISTS ix_events_ts ON events(ts DESC);
CREATE INDEX IF NOT EXISTS ix_events_type_ts ON events(type, ts DESC);
CREATE INDEX IF NOT EXISTS ix_events_device_ts ON events(device_id, ts DESC);

-- RBAC par site: un user peut être admin sur Site A, viewer sur Site B
CREATE TABLE IF NOT EXISTS iot.user_site_roles (
  tenant_id uuid NOT NULL REFERENCES iot.tenants(id) ON DELETE CASCADE,
  user_id uuid NOT NULL REFERENCES iot.users(id) ON DELETE CASCADE,
  site_id uuid NOT NULL REFERENCES iot.sites(id) ON DELETE CASCADE,
  role text NOT NULL, -- admin|operator|viewer
  created_at timestamptz NOT NULL DEFAULT now(),
  PRIMARY KEY (tenant_id, user_id, site_id)
);

CREATE INDEX IF NOT EXISTS ix_usr_site_roles_site ON iot.user_site_roles(tenant_id, site_id);

-- trigger updated_at (si pas déjà)
CREATE OR REPLACE FUNCTION iot.set_updated_at() RETURNS trigger LANGUAGE plpgsql AS $$
BEGIN NEW.updated_at = now(); RETURN NEW; END $$;

DO $$ BEGIN
  CREATE TRIGGER trg_users_updated BEFORE UPDATE ON iot.users
  FOR EACH ROW EXECUTE FUNCTION iot.set_updated_at();
EXCEPTION WHEN duplicate_object THEN NULL; END $$;

-- NOTIFY (optionnel) si tu veux resync des permissions
CREATE OR REPLACE FUNCTION iot.notify_rbac() RETURNS trigger LANGUAGE plpgsql AS $$
BEGIN
  PERFORM pg_notify('iot_rbac', json_build_object(
    'table', TG_TABLE_NAME,
    'op', TG_OP,
    'tenant_id', COALESCE(NEW.tenant_id, OLD.tenant_id)::text,
    'user_id', COALESCE(NEW.user_id, OLD.user_id)::text
  )::text);
  RETURN COALESCE(NEW, OLD);
END $$;

DO $$ BEGIN
  CREATE TRIGGER trg_user_site_roles_notify
  AFTER INSERT OR UPDATE OR DELETE ON iot.user_site_roles
  FOR EACH ROW EXECUTE FUNCTION iot.notify_rbac();
EXCEPTION WHEN duplicate_object THEN NULL; END $$;


-- ============================================================
-- 14) LISTEN/NOTIFY : topologie + commandes
-- ============================================================

CREATE OR REPLACE FUNCTION notify_topology_change() RETURNS trigger
LANGUAGE plpgsql AS $$
DECLARE
  payload json;
BEGIN
  payload := json_build_object(
    'table', TG_TABLE_NAME,
    'op', TG_OP,
    'id', COALESCE(
      (CASE WHEN TG_OP='DELETE' THEN OLD.id ELSE NEW.id END)::text,
      NULL
    ),
    'ts', now()
  );
  PERFORM pg_notify('iot_topology', payload::text);
  RETURN COALESCE(NEW, OLD);
END $$;

CREATE OR REPLACE FUNCTION notify_command_change() RETURNS trigger
LANGUAGE plpgsql AS $$
DECLARE
  payload json;
BEGIN
  payload := json_build_object(
    'table', TG_TABLE_NAME,
    'op', TG_OP,
    'id', COALESCE((CASE WHEN TG_OP='DELETE' THEN OLD.id ELSE NEW.id END)::text, NULL),
    'device_id', COALESCE(
      (CASE WHEN TG_OP='DELETE' THEN OLD.device_id ELSE NEW.device_id END)::text,
      NULL
    ),
    'status', COALESCE(
      (CASE WHEN TG_OP='DELETE' THEN OLD.status ELSE NEW.status END)::text,
      NULL
    ),
    'ts', now()
  );
  PERFORM pg_notify('iot_commands', payload::text);
  RETURN COALESCE(NEW, OLD);
END $$;

-- Triggers topologie
DO $$ BEGIN
  CREATE TRIGGER trg_sites_notify AFTER INSERT OR UPDATE OR DELETE ON sites
  FOR EACH ROW EXECUTE FUNCTION notify_topology_change();
EXCEPTION WHEN duplicate_object THEN NULL; END $$;

DO $$ BEGIN
  CREATE TRIGGER trg_zones_notify AFTER INSERT OR UPDATE OR DELETE ON zones
  FOR EACH ROW EXECUTE FUNCTION notify_topology_change();
EXCEPTION WHEN duplicate_object THEN NULL; END $$;

DO $$ BEGIN
  CREATE TRIGGER trg_mqtt_servers_notify AFTER INSERT OR UPDATE OR DELETE ON mqtt_servers
  FOR EACH ROW EXECUTE FUNCTION notify_topology_change();
EXCEPTION WHEN duplicate_object THEN NULL; END $$;

DO $$ BEGIN
  CREATE TRIGGER trg_modbus_servers_notify AFTER INSERT OR UPDATE OR DELETE ON modbus_servers
  FOR EACH ROW EXECUTE FUNCTION notify_topology_change();
EXCEPTION WHEN duplicate_object THEN NULL; END $$;

DO $$ BEGIN
  CREATE TRIGGER trg_opcua_servers_notify AFTER INSERT OR UPDATE OR DELETE ON opcua_servers
  FOR EACH ROW EXECUTE FUNCTION notify_topology_change();
EXCEPTION WHEN duplicate_object THEN NULL; END $$;

DO $$ BEGIN
  CREATE TRIGGER trg_devices_notify AFTER INSERT OR UPDATE OR DELETE ON devices
  FOR EACH ROW EXECUTE FUNCTION notify_topology_change();
EXCEPTION WHEN duplicate_object THEN NULL; END $$;

DO $$ BEGIN
  CREATE TRIGGER trg_topics_notify AFTER INSERT OR UPDATE OR DELETE ON device_topics
  FOR EACH ROW EXECUTE FUNCTION notify_topology_change();
EXCEPTION WHEN duplicate_object THEN NULL; END $$;

DO $$ BEGIN
  CREATE TRIGGER trg_commands_notify AFTER INSERT ON device_commands
  FOR EACH ROW EXECUTE FUNCTION notify_command_change();
EXCEPTION WHEN duplicate_object THEN NULL; END $$;
