#pragma once

#include <Repositories/GenericRepo.hpp>
#include <Factory/MqttTopicBuilder.hpp>

namespace FalcataIoTServer
{
    struct MqttTopicRepo
    {
        using Factory = MqttTopicFactory;

        static std::string SelectAllSql() {
            return R"SQL(
                SELECT
                  t.id,
                  t.device_id,
                  t.role,
                  t.topic,
                  t.qos,
                  t.retain,
                  t.direction::text AS direction,
                  t.enabled,
                  t.metadata,
                  t.created_at,
                  t.updated_at
                FROM iot.device_topics t
            )SQL";
        }

        static std::string SelectByIdSql() { return SelectAllSql() + " WHERE id = $1"; }

        // discriminator = device_id
        static std::string SelectByDiscSql() {
            return SelectAllSql() + " WHERE protocol = $1 ORDER BY role";
        }

        static std::string InsertSql() {
            return R"SQL("INSERT INTO iot.device_topics "
                   "(id, device_id, role, topic, qos, retain, direction, enabled, metadata) "
                   "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9::jsonb)")SQL";
        }

        static std::string UpdateSql() {
            return R"SQL("UPDATE iot.device_topics SET "
                   "device_id=$2, role=$3, topic=$4, qos=$5, retain=$6, direction=$7, enabled=$8,
                   " "metadata=$9::jsonb " "WHERE id=$1")SQL";
        }

        static std::string DeleteSql() {
            return R"SQL("DELETE FROM iot.device_topics WHERE id=$1")SQL";
        }
    };

    using MqttTopicRepository = GenericRepo<MqttTopicRepo>;
}  // namespace FalcataIoTServer
