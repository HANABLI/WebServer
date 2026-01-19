/**
 * @file Zone.cpp
 */
#include <Models/Location/Zone.hpp>
#include <memory>
namespace FalcataIoTServer
{
    struct Zone::Impl
    {
        std::string siteId;
        std::string name;
        std::string description;
        std::string kind;
        Json::Value geojson = Json::Value(Json::Value::Type::Object);

        std::vector<std::string> tags;
        Json::Value metadata = Json::Value(Json::Value::Type::Object);

        std::string createdAt;
        std::string updatedAt;
        // transient
        std::vector<std::string> deviceIds;
    };

    Zone::Zone() : CoreObject(), impl_(std::make_unique<Impl>()) {}
    Zone::Zone(const Json::Value& j) : CoreObject(), impl_(std::make_unique<Impl>()) {
        FromJson(j);
    }
    Zone::~Zone() noexcept = default;

    const std::string& Zone::GetSiteId() const { return impl_->siteId; }
    void Zone::SetSiteId(const std::string& v) { impl_->siteId = v; }

    const std::string& Zone::GetName() const { return impl_->name; }
    void Zone::SetName(const std::string& v) { impl_->name = v; }

    const std::string& Zone::GetDescription() const { return impl_->description; }
    void Zone::SetDescription(const std::string& v) { impl_->description = v; }

    const std::string& Zone::GetKind() const { return impl_->kind; }
    void Zone::SetKind(const std::string& v) { impl_->kind = v; }

    const Json::Value& Zone::GetGeoJson() const { return impl_->geojson; }
    void Zone::SetGeoJson(const Json::Value& v) { impl_->geojson = v; }

    const std::vector<std::string>& Zone::GetTags() const { return impl_->tags; }
    void Zone::SetTags(std::vector<std::string> v) { impl_->tags = std::move(v); }
    void Zone::AddTag(const std::string& tag) { impl_->tags.push_back(tag); }

    const Json::Value& Zone::GetMetadata() const { return impl_->metadata; }
    void Zone::SetMetadata(const Json::Value& v) { impl_->metadata = v; }

    const std::string& Zone::GetCreatedAt() const { return impl_->createdAt; }
    void Zone::SetCreatedAt(const std::string& v) { impl_->createdAt = v; }

    const std::string& Zone::GetUpdatedAt() const { return impl_->updatedAt; }
    void Zone::SetUpdatedAt(const std::string& v) { impl_->updatedAt = v; }

    const std::vector<std::string>& Zone::GetDeviceIds() const { return impl_->deviceIds; }
    void Zone::SetDeviceIds(std::vector<std::string> ids) { impl_->deviceIds = std::move(ids); }
    void Zone::AddDeviceId(const std::string& deviceId) { impl_->deviceIds.push_back(deviceId); }

    Json::Value Zone::ToJson() const {
        Json::Value j(Json::Value::Type::Object);
        j.Set("id", Uuid_s());
        j.Set("site_id", impl_->siteId);
        j.Set("name", impl_->name);
        if (!impl_->description.empty())
            j.Set("description", impl_->description);
        if (!impl_->kind.empty())
            j.Set("kind", impl_->kind);
        j.Set("geojson", impl_->geojson);

        Json::Value tags(Json::Value::Type::Array);
        for (size_t i = 0; i < impl_->tags.size(); ++i)
        { tags.Add(impl_->tags.at(i)); }
        j.Set("tags", tags);

        j.Set("metadata", impl_->metadata);

        if (!impl_->createdAt.empty())
            j.Set("created_at", impl_->createdAt);
        if (!impl_->updatedAt.empty())
            j.Set("updated_at", impl_->updatedAt);

        // transient

        if (!impl_->deviceIds.empty())
        {
            Json::Value devices(Json::Value::Type::Array);
            for (size_t i = 0; i < impl_->deviceIds.size(); ++i)
            { devices.Add(impl_->deviceIds.at(i)); }
            j.Set("device_ids", devices);
        }
        return j;
    }

    void Zone::FromJson(const Json::Value& j) {
        if (j.Has("id"))
            UuidFromString(j["id"].ToEncoding());
        if (j.Has("site_id"))
            impl_->siteId = j["site_id"];
        if (j.Has("name"))
            impl_->name = j["name"];
        if (j.Has("description"))
            impl_->description = j["description"];
        if (j.Has("kind"))
            impl_->kind = j["kind"];
        if (j.Has("geojson"))
            impl_->geojson = j["geojson"];

        if (j.Has("tags") && (j["tags"].GetType() == Json::Value::Type::Array))
        {
            impl_->tags.clear();
            for (size_t i = 0; i < j["tags"].GetSize(); ++i)
                impl_->tags.push_back(j["tags"][i].ToEncoding());
        }

        if (j.Has("metadata"))
            impl_->metadata = j["metadata"];
        if (j.Has("created_at"))
            impl_->createdAt = j["created_at"];
        if (j.Has("updated_at"))
            impl_->updatedAt = j["updated_at"];

        // transient

        if (j.Has("device_ids") && (j["device_ids"].GetType() == Json::Value::Type::Array))
        {
            impl_->deviceIds.clear();
            for (size_t i = 0; i < j["device_ids"].GetSize(); ++i)
                impl_->deviceIds.push_back(j["device_ids"][i].ToEncoding());
        }
    }

    const std::vector<std::string> Zone::GetInsertParams() const { return {""}; }
    const std::vector<std::string> Zone::GetUpdateParams() const { return {""}; }
    const std::vector<std::string> Zone::GetRemoveParams() const { return {""}; }
    const std::vector<std::string> Zone::GetDisableParams() const { return {""}; }
}  // namespace FalcataIoTServer