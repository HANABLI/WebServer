#pragma once
#include <Repositories/GenericRepo.hpp>
#include <Factory/ServerBuilder.hpp>

namespace FalcataIoTServer
{
    struct ServerRepo
    {
        using Factory = ServerFactory;

        static std::string SelectAllSql() {
            return R"SQL(
                SELECT
                  s.id,
                  s.name,
                  s.protocol::text AS protocol,
                  s.enabled,
                  s.host,
                  s.port,
                  s.use_tls AS "useTLS",

                  -- MQTT options in metadata
                  COALESCE((s.metadata->>'cleanSession')::boolean, true)  AS "cleanSession",
                  COALESCE((s.metadata->>'willRetain')::boolean, false)  AS "willRetain",
                  COALESCE((s.metadata->>'willTopic')::text, '')         AS "willTopic",
                  COALESCE((s.metadata->>'willPayload')::text, '')       AS "willPayload",
                  COALESCE((s.metadata->>'qos')::int, 1)                 AS "qos",
                  COALESCE((s.metadata->>'keepAlive')::int, 30)          AS "keepAlive",

                  -- credentials (ServerBuilder expects userName/password)
                  COALESCE(c.username, '')      AS "userName",
                  COALESCE(encode(c.password_enc, 'escape'), '') AS "password"

                FROM iot.servers s
                LEFT JOIN iot.server_credentials c ON c.server_id = s.id
            )SQL";
        }

        static std::string SelectByIdSql() { return SelectAllSql() + " WHERE id = $1"; }

        static std::string SelectByDiscSql() {
            // discriminator = protocol
            return SelectAllSql() + " WHERE protocol = $1";
        }
    };

    using ServerRepository = GenericRepo<ServerRepo>;
}  // namespace FalcataIoTServer
