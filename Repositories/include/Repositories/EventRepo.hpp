#pragma once
#include <Repositories/GenericRepo.hpp>
#include <Factory/EventBuilder.hpp>

namespace FalcataIoTServer
{
    struct EventRepo
    {
        using Factory = EventFactory;

        static std::string SelectAllSql() {
            return R"SQL(
                SELECT
                  e.id,
                  e.ts,
                  e.source::text AS source,
                  e.site_id,
                  e.zone_id,
                  e.device_id,
                  e.type,
                  e.severity::text AS severity,
                  e.correlation_id,
                  e.payload
                FROM iot.events e
            )SQL";
        }

        static std::string SelectByIdSql() { return SelectAllSql() + " WHERE id = $1"; }

        static std::string SelectByDiscSql() {
            // discriminator = type
            return SelectAllSql() + " WHERE type = $1 ORDER BY ts DESC";
        }
    };

    using EventRepository = GenericRepo<EventRepo>;
}  // namespace FalcataIoTServer
