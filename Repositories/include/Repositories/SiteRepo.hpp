#pragma once
#include <Repositories/GenericRepo.hpp>
#include <Factory/SiteBuilder.hpp>

namespace FalcataIoTServer
{
    struct SiteRepo
    {
        using Factory = SiteFactory;

        static std::string SelectAllSql() {
            return R"SQL(
                SELECT
                  s.id,
                  s.name,
                  s.description,
                  s.kind,
                  s.country,
                  s.timezone,
                  s.tags,
                  s.metadata,
                  s.created_at,
                  s.updated_at
                FROM iot.sites s
            )SQL";
        }

        static std::string SelectByIdSql() { return SelectAllSql() + " WHERE id = $1"; }

        static std::string SelectByDiscSql() {
            // discriminator = kind ("farm"/"city"/...)
            return SelectAllSql() + " WHERE kind = $1";
        }
    };

    using SiteRepository = GenericRepo<SiteRepo>;
}  // namespace FalcataIoTServer
