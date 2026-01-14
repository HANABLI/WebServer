/**
 * @file Tenant.cpp
 * @brief This is the declaration of the FalcataIoTServer::Tenant class.
 * @copyright copyright © 2026 by Hatem Nabli.
 */
#include <Models/Auth/Tenant.hpp>
#include <memory>
#include <string>

namespace FalcataIoTServer
{
    struct Tenant::Impl
    {
        std::string name;
        std::string slug;
        std::string createdAt;
    };

    Tenant::Tenant() : CoreObject(), impl_(std::make_unique<Impl>()) {}

    const std::string& Tenant::GetTenantName() const { return impl_->name; }
    void Tenant::SetTenantName(const std::string& name) { impl_->name = name; }

    const std::string& Tenant::GetTenantSlug() const { return impl_->slug; }
    void Tenant::SetTenantSlug(const std::string& slug) { impl_->slug = slug; }

    const std::vector<std::string> Tenant::GetInsertParams() const { return {}; }
    const std::vector<std::string> Tenant::GetUpdateParams() const { return {}; }
    const std::vector<std::string> Tenant::GetRemoveParams() const { return {}; }
    const std::vector<std::string> Tenant::GetDisableParams() const { return {}; }

    // JSON
    Json::Value Tenant::ToJson() const {
        Json::Value j(Json::Value::Type::Object);
        j.Set("id", Uuid_s());
        j.Set("name", GetTenantName());
        j.Set("slug", GetTenantSlug());
        return j;
    }

    void Tenant::FromJson(const Json::Value& json) {
        if (json.Has("id"))
        { UuidFromString(json["id"]); }
        if (json.Has("name"))
        { SetTenantName(json["name"]); }
        if (json.Has("slug"))
        { SetTenantSlug(json["slug"]); }
    }

}  // namespace FalcataIoTServer