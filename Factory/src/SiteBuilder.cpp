
#include <Factory/SiteBuilder.hpp>

namespace FalcataIoTServer
{
    std::string SiteBuilder::Discriminator(const Postgresql::PgResult& res, int row) {
        return res.TextRequired(row, "site_id");
    }

    std::unique_ptr<SiteBuilder::Base> SiteBuilder::Build(const std::string&,
                                                          const Postgresql::PgResult& res,
                                                          int row) {
        auto s = std::make_unique<Site>();
        s->UuidFromString(res.TextRequired(row, "id"));
        s->SetName(res.TextRequired(row, "name"));
        s->SetKind(res.TextRequired(row, "kind"));
        s->SetCountry(res.TextRequired(row, "country"));
        s->SetTimezone(res.TextRequired(row, "timezone"));
        s->SetCreatedAt(res.TextRequired(row, "created_at"));
        s->SetUpdatedAt(res.TextRequired(row, "updated_at"));
        s->SetDescription(res.TextRequired(row, "description"));
        s->SetMetadata(res.Json(row, "metadata", Json::Value::Type::Object));
        s->SetTags({res.Json(row, "tags", Json::Value::Type::Array)});
        s->SetZoneIds({res.Json(row, "zone_ids", Json::Value::Type::Array)});

        return s;
    }
}  // namespace FalcataIoTServer
