#include <Factory/ZoneBuilder.hpp>

namespace FalcataIoTServer
{
    std::string ZoneBuilder::Discriminator(const Postgresql::PgResult& res, int row) {
        return res.TextRequired(row, "site_id");
    }

    std::unique_ptr<ZoneBuilder::Base> ZoneBuilder::Build(const std::string&,
                                                          const Postgresql::PgResult& res,
                                                          int row) {
        auto z = std::make_unique<Zone>();
        z->UuidFromString(res.TextRequired(row, "id"));
        z->SetName(res.TextRequired(row, "name"));
        z->SetKind(res.TextRequired(row, "kind"));
        z->SetSiteId(res.TextRequired(row, "site_id"));
        z->SetCreatedAt(res.TextRequired(row, "created_at"));
        z->SetUpdatedAt(res.TextRequired(row, "updated_at"));
        z->SetDescription(res.TextRequired(row, "description"));
        z->SetMetadata(res.Json(row, "metadata", Json::Value::Type::Object));
        z->SetTags({res.Json(row, "tags", Json::Value::Type::Array)});
        z->SetDeviceIds({res.Json(row, "device_ids", Json::Value::Type::Array)});

        return z;
    }
}  // namespace FalcataIoTServer