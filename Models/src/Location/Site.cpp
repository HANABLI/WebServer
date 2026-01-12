/**
 * @file Site.cpp
 */
#include <Models/Location/Site.hpp>
#include <memory>
namespace FalcataIoTServer
{
    struct Site::Impl
    {
        std::string name;
        std::string description;
        std::string kind = "site";
        std::string country;
        std::string timezone = "Europe/Paris";

        std::vector<std::string> tags;
        Json::Value metadata = Json::Value(Json::Value::Type::Object);

        std::string createdAt;
        std::string updatedAt;

        // transient
        std::vector<std::string> zoneIds;
    };

    Site::Site() : CoreObject(), impl_(std::make_unique<Impl>()) {}
    Site::Site(const Json::Value& j) : CoreObject(), impl_(std::make_unique<Impl>()) {
        FromJson(j);
    }

    const std::string& Site::GetName() const { return impl_->name; }
    void Site::SetName(const std::string& v) { impl_->name = v; }

    const std::string& Site::GetDescription() const { return impl_->description; }
    void Site::SetDescription(const std::string& v) { impl_->description = v; }

    const std::string& Site::GetKind() const { return impl_->kind; }
    void Site::SetKind(const std::string& v) { impl_->kind = v; }

    const std::string& Site::GetCountry() const { return impl_->country; }
    void Site::SetCountry(const std::string& v) { impl_->country = v; }

    const std::string& Site::GetTimezone() const { return impl_->timezone; }
    void Site::SetTimezone(const std::string& v) { impl_->timezone = v; }

    const std::vector<std::string>& Site::GetTags() const { return impl_->tags; }
    void Site::SetTags(std::vector<std::string> v) { impl_->tags = std::move(v); }
    void Site::AddTag(const std::string& tag) { impl_->tags.push_back(tag); }

    const Json::Value& Site::GetMetadata() const { return impl_->metadata; }
    void Site::SetMetadata(const Json::Value& v) { impl_->metadata = v; }

    const std::string& Site::GetCreatedAt() const { return impl_->createdAt; }
    void Site::SetCreatedAt(const std::string& v) { impl_->createdAt = v; }

    const std::string& Site::GetUpdatedAt() const { return impl_->updatedAt; }
    void Site::SetUpdatedAt(const std::string& v) { impl_->updatedAt = v; }

    const std::vector<std::string>& Site::GetZoneIds() const { return impl_->zoneIds; }

    void Site::SetZoneIds(std::vector<std::string> ids) { impl_->zoneIds = std::move(ids); }

    void Site::AddZoneId(const std::string& zoneId) { impl_->zoneIds.push_back(zoneId); }

    Json::Value Site::ToJson() const {
        Json::Value j(Json::Value::Type::Object);
        j.Set("id", Uuid_s());
        j.Set("name", impl_->name);

        if (!impl_->description.empty())
            j.Set("description", impl_->description);
        j.Set("kind", impl_->kind);
        if (!impl_->country.empty())
            j.Set("country", impl_->country);
        if (!impl_->timezone.empty())
            j.Set("timezone", impl_->timezone);

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
        if (!impl_->zoneIds.empty())
        {
            Json::Value zones(Json::Value::Type::Array);
            for (size_t i = 0; i < impl_->zoneIds.size(); ++i)
            { zones.Add(impl_->zoneIds.at(i)); }
            j.Set("zone_ids", zones);
        }

        return j;
    }

    void Site::FromJson(const Json::Value& j) {
        if (j.Has("id"))
            UuidFromString(j["id"]);
        if (j.Has("name"))
            impl_->name = j["name"];
        if (j.Has("kind"))
            impl_->kind = j["kind"];
        if (j.Has("description"))
            impl_->description = j["description"];
        if (j.Has("country"))
            impl_->country = j["country"];
        if (j.Has("timezone"))
            impl_->timezone = j["timezone"];
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
        if (j.Has("zone_ids") && (j["zone_ids"].GetType() == Json::Value::Type::Array))
        {
            impl_->zoneIds.clear();
            for (size_t i = 0; i < j["zone_ids"].GetSize(); ++i)
                impl_->zoneIds.push_back(j["zone_ids"][i].ToEncoding());
        }
    }

    const std::vector<std::string> Site::GetInsertParams() const { return {""}; }
    const std::vector<std::string> Site::GetUpdateParams() const { return {""}; }
    const std::vector<std::string> Site::GetRemoveParams() const { return {""}; }
    const std::vector<std::string> Site::GetDisableParams() const { return {""}; }

}  // namespace FalcataIoTServer