#pragma once
#include <Repositories/GenericRepo.hpp>
#include <Factory/IoTDeviceBuilder.hpp>

namespace FalcataIoTServer
{
    struct IoTDeviceRepo
    {
        using Factory = IoTDeviceFactory;

        static std::string SelectAllSql() {
            return R"SQL(
                SELECT
                    d.id,
                    d.name,
                    d.kind,
                    d.protocol::text AS protocol,
                    d.enabled,

                    d.site_id,
                    d.zone_id,
                    d.type_id,
                    d.server_id,

                    d.external_id,
                    d.last_seen_at,

                    d.tags,
                    d.metadata
                FROM iot.devices d
            )SQL";
        }

        static std::string SelectByIdSql() { return SelectAllSql() + " WHERE id = $1"; }

        static std::string SelectByDiscSql() {
            // discriminator = protocol OR type_id (choose one; here protocol)
            return SelectAllSql() + " WHERE protocol = $1";
        }
    };

    using IoTDeviceRepository = GenericRepo<IoTDeviceRepo>;
}  // namespace FalcataIoTServer
