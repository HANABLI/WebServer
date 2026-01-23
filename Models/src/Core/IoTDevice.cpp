#include <Models/Core/IoTDevice.hpp>

namespace FalcataIoTServer
{
    IoTDevice::IoTDevice() : CoreObject(), impl_(std::make_unique<Impl>()) {}
    IoTDevice::IoTDevice(const Json::Value& j) : CoreObject(), impl_(std::make_unique<Impl>()) {
        FromJson(j);
    }
    IoTDevice::IoTDevice(std::string id, std::string serverId, std::string name, std::string kind,
                         std::string protocol, bool enabled, std::string zoneId) :
        CoreObject(std::move(id)), impl_(std::make_unique<Impl>()) {
        SetId(id);
        impl_->serverId = std::move(serverId);
        impl_->name = std::move(name);
        impl_->kind = std::move(kind);
        impl_->protocol = std::move(protocol);
        impl_->enabled = enabled;
        impl_->zoneId = std::move(zoneId);
    }

    IoTDevice::IoTDevice(std::string name, std::string kind, std::string protocol, bool enabled,
                         std::string zoneId) :
        CoreObject(), impl_(std::make_unique<Impl>()) {
        impl_->name = std::move(name);
        impl_->kind = std::move(kind);
        impl_->protocol = std::move(protocol);
        impl_->enabled = enabled;
        impl_->zoneId = std::move(zoneId);
    }

    const std::string IoTDevice::GetServerId() const { return impl_->serverId; }
    void IoTDevice::SetServerId(const std::string& serverId) { impl_->serverId = serverId; }

    const std::string IoTDevice::GetZone() const { return impl_->zoneId; }
    void IoTDevice::SetZone(const std::string& zoneId) { impl_->zoneId = zoneId; }

    const std::vector<std::string> IoTDevice::GetEvents() const { return impl_->events; }
    void IoTDevice::SetEvents(const std::vector<std::string>& eventIds) {
        impl_->events = eventIds;
    }
    void IoTDevice::AddEvent(const std::string& eventId) { impl_->events.push_back(eventId); }

    const std::string& IoTDevice::GetSiteId() const { return impl_->siteId; }
    void IoTDevice::SetSiteId(const std::string& v) { impl_->siteId = v; }

    const std::string& IoTDevice::GetTypeId() const { return impl_->typeId; }
    void IoTDevice::SetTypeId(const std::string& v) { impl_->typeId = v; }

    const std::string& IoTDevice::GetExternalId() const { return impl_->externalId; }
    void IoTDevice::SetExternalId(const std::string& v) { impl_->externalId = v; }

    const std::string& IoTDevice::GetLastSeenAt() const { return impl_->lastSeenAt; }
    void IoTDevice::SetLastSeenAt(const std::string& v) { impl_->lastSeenAt = v; }

    const std::vector<std::string>& IoTDevice::GetTags() const { return impl_->tags; }
    void IoTDevice::SetTags(std::vector<std::string> v) { impl_->tags = std::move(v); }
    void IoTDevice::AddTag(const std::string& tag) { impl_->tags.push_back(tag); }

    const Json::Value& IoTDevice::GetMetadata() const { return impl_->metadata; }
    void IoTDevice::SetMetadata(const Json::Value& v) { impl_->metadata = v; }

    const std::string& IoTDevice::GetCreatedAt() const { return impl_->createdAt; }
    void IoTDevice::SetCreatedAt(const std::string& v) { impl_->createdAt = v; }

    const std::string& IoTDevice::GetUpdatedAt() const { return impl_->updatedAt; }
    void IoTDevice::SetUpdatedAt(const std::string& v) { impl_->updatedAt = v; }

    void IoTDevice::FromJson(const Json::Value& j) {
        Device<IoTDevice>::FromJson(j);

        if (j.Has("site_id"))
            SetSiteId(j["site_id"]);
        if (j.Has("zone_id"))
            SetZone(j["zone_id"]);
        if (j.Has("type_id"))
            SetTypeId(j["type_id"]);
        if (j.Has("server_id"))
            SetServerId(j["server_id"]);

        if (j.Has("external_id"))
            SetExternalId(j["external_id"]);
        if (j.Has("last_seen_at"))
            SetLastSeenAt(j["last_seen_at"]);

        if (j.Has("tags") && (j["tags"].GetType() == Json::Value::Type::Array))
        {
            impl_->tags.clear();
            for (size_t i = 0; i < j["tags"].GetSize(); ++i)
                impl_->tags.push_back(j["tags"][i].ToEncoding());
        }

        if (j.Has("metadata"))
            SetMetadata(j["metadata"]);
        if (j.Has("created_at"))
            SetCreatedAt(j["created_at"]);
        if (j.Has("updated_at"))
            SetUpdatedAt(j["updated_at"]);

        if (j.Has("event_ids") && (j["event_ids"].GetType() == Json::Value::Type::Array))
        {
            impl_->events.clear();
            for (size_t i = 0; i < j["event_ids"].GetSize(); ++i)
                impl_->tags.push_back(j["event_ids"][i].ToEncoding());
        }
    }

    Json::Value IoTDevice::ToJson() const {
        Json::Value j = Device<IoTDevice>::ToJson();

        j.Set("site_id", impl_->siteId);
        j.Set("zone_id", impl_->zoneId);
        j.Set("type_id", impl_->typeId);
        j.Set("server_id", impl_->serverId);

        if (!impl_->externalId.empty())
            j.Set("external_id", impl_->externalId);
        if (!impl_->lastSeenAt.empty())
            j.Set("last_seen_at", impl_->lastSeenAt);

        Json::Value tags(Json::Value::Type::Array);
        for (size_t i = 0; i < impl_->tags.size(); ++i)
        { tags.Add(impl_->tags.at(i)); }
        j.Set("tags", tags);

        j.Set("metadata", impl_->metadata);

        if (!impl_->createdAt.empty())
            j.Set("created_at", impl_->createdAt);
        if (!impl_->updatedAt.empty())
            j.Set("updated_at", impl_->updatedAt);

        // existing transient
        if (!impl_->events.empty())
        {
            Json::Value ev(Json::Value::Type::Array);
            for (size_t i = 0; i < impl_->events.size(); ++i)
            { ev.Add(impl_->events.at(i)); }
            j.Set("event_ids", ev);
        }
        return j;
    }

    const std::vector<std::string> IoTDevice::GetInsertParams() const { return {""}; }
    const std::vector<std::string> IoTDevice::GetUpdateParams() const { return {""}; }
    const std::vector<std::string> IoTDevice::GetRemoveParams() const { return {""}; }
    const std::vector<std::string> IoTDevice::GetDisableParams() const { return {""}; }
}  // namespace FalcataIoTServer