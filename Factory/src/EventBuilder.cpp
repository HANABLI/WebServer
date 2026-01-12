
#include <Factory/EventBuilder.hpp>

namespace FalcataIoTServer
{
    std::string EventBuilder::Discriminator(const Postgresql::PgResult& res, int row) {
        return res.TextRequired(row, "device_id");
    }

    std::unique_ptr<EventBuilder::Base> EventBuilder::Build(const std::string&,
                                                            const Postgresql::PgResult& res,
                                                            int row) {
        auto e = std::make_unique<Event>();
        e->CorrelationId(res.TextRequired(row, "correlation_id"));
        e->DeviceId(res.TextRequired(row, "device_id"));
        e->SiteId(res.TextRequired(row, "site_id"));
        e->ZoneId(res.TextRequired(row, "zone_id"));
        e->Ts(res.TextRequired(row, "ts"));
        e->Source(res.TextRequired(row, "source"));
        e->Type(res.TextRequired(row, "type"));
        e->Severity(res.TextRequired(row, "severity"));
        e->Payload(res.Json(row, "payload", Json::Value::Type::Object));
        e->UuidFromString(res.TextRequired(row, "id"));
        e->SetCreatedAt(res.TextRequired(row, "created_at"));

        return e;
    }
}  // namespace FalcataIoTServer
