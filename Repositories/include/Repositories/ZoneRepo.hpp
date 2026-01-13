#pragma once
#include <Repositories/GenericRepo.hpp>
#include <Factory/ZoneBuilder.hpp>

namespace FalcataIoTServer
{
    struct ZoneRepo
    {
        using Factory = ZoneFactory;

        static std::string SelectAllSql() {
            return R"SQL(
                SELECT
                  z.id,
                  z.site_id,
                  z.name,
                  z.description,
                  z.kind,
                  z.geojson,
                  z.tags,
                  z.metadata,
                  z.created_at,
                  z.updated_at
                FROM iot.zones z
            )SQL";
        }

        static std::string SelectByIdSql() { return SelectAllSql() + " WHERE id = $1"; }

        static std::string SelectByDiscSql() {
            // discriminator = site_id
            return SelectAllSql() + " WHERE site_id = $1::uuid";
        }
    };

    using ZoneRepository = GenericRepo<ZoneRepo>;
}  // namespace FalcataIoTServer
