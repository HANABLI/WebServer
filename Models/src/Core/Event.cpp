
#include <Models/Core/Event.hpp>

namespace FalcataIoTServer
{
    struct Event::Impl
    {
        std::string ts;                 // ISO-8601 timestamptz string. Ex: "2025-12-16T12:00:00Z"
        std::string source = "system";  // event_source enum: iot|vision|ai|system|user
        std::string type;               // ex: "device.online", "ai.anomaly"
        std::string severity = "info";  // severity_level enum
        std::string siteId, zoneId, deviceId, cameraId;
        std::string correlationId;  // uuid string
        Json::Value payload = Json::Value(Json::Value::Type::Object);
        std::string createdAt;
        std::string updatedAt;
    };

    Event::Event() : CoreObject(), impl_(std::make_unique<Impl>()) {}

    const std::string& Event::Ts() const { return impl_->ts; }
    void Event::Ts(const std::string& v) { impl_->ts = v; }

    const std::string& Event::Source() const { return impl_->source; }
    void Event::Source(const std::string& v) { impl_->source = v; }

    const std::string& Event::Type() const { return impl_->type; }
    void Event::Type(const std::string& v) { impl_->type = v; }

    const std::string& Event::Severity() const { return impl_->severity; }
    void Event::Severity(const std::string& v) { impl_->severity = v; }

    const std::string& Event::SiteId() const { return impl_->siteId; }
    void Event::SiteId(const std::string& v) { impl_->siteId = v; }

    const std::string& Event::ZoneId() const { return impl_->zoneId; }
    void Event::ZoneId(const std::string& v) { impl_->zoneId = v; }

    const std::string& Event::DeviceId() const { return impl_->deviceId; }
    void Event::DeviceId(const std::string& v) { impl_->deviceId = v; }

    const std::string& Event::CorrelationId() const { return impl_->correlationId; }
    void Event::CorrelationId(const std::string& v) { impl_->correlationId = v; }

    const Json::Value& Event::Payload() const { return impl_->payload; }
    void Event::Payload(const Json::Value& v) { impl_->payload = v; }

    const std::string& Event::GetCreatedAt() const { return impl_->createdAt; }
    void Event::SetCreatedAt(const std::string& v) { impl_->createdAt = v; }

    Json::Value Event::ToJson() const {
        Json::Value j(Json::Value::Type::Object);
        j.Set("id", Uuid_s());
        if (!impl_->ts.empty())
            j.Set("ts", impl_->ts);
        j.Set("source", impl_->source);
        j.Set("type", impl_->type);
        j.Set("severity", impl_->severity);
        if (!impl_->siteId.empty())
            j.Set("site_id", impl_->siteId);
        if (!impl_->zoneId.empty())
            j.Set("zone_id", impl_->zoneId);
        if (!impl_->deviceId.empty())
            j.Set("device_id", impl_->deviceId);
        if (!impl_->correlationId.empty())
            j.Set("correlation_id", impl_->correlationId);
        j.Set("payload", impl_->payload);
        return j;
    }

    void Event::FromJson(const Json::Value& j) {
        if (j.Has("id"))
            UuidFromString(j["id"]);
        if (j.Has("ts"))
            Ts(j["ts"]);
        if (j.Has("source"))
            Source(j["source"]);
        if (j.Has("type"))
            Type(j["type"]);
        if (j.Has("severity"))
            Severity(j["severity"]);
        if (j.Has("site_id"))
            SiteId(j["site_id"]);
        if (j.Has("zone_id"))
            ZoneId(j["zone_id"]);
        if (j.Has("device_id"))
            DeviceId(j["device_id"]);
        if (j.Has("correlation_id"))
            CorrelationId(j["correlation_id"]);
        if (j.Has("payload"))
            Payload(j["payload"]);
    }

    const std::vector<std::string> Event::GetInsertParams() const { return {""}; }
    const std::vector<std::string> Event::GetUpdateParams() const { return {""}; }
    const std::vector<std::string> Event::GetRemoveParams() const { return {""}; }
    const std::vector<std::string> Event::GetDisableParams() const { return {""}; }
}  // namespace FalcataIoTServer