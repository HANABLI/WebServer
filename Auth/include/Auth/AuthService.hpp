#pragma once
/**
 * @file AuthService.hpp
 * @brief This represent the declaration of Auth::IAuthService interface.
 * @copyright copyright © 2025 by Hatem Nabli.
 */
#include <Auth/Role.hpp>
#include <Json/Json.hpp>
#include <optional>

namespace Auth
{
    struct Identity
    {
        std::string sub;          // username or user id
        std::string tenant_slug;  // tenant scope
        std::string tenant_id;
        Role role = Role::Viewer;

        std::vector<std::string> site_ids;

        Json::Value claims;
    };

    class IAuthService
    {
    public:
        virtual ~IAuthService() = default;
        virtual std::optional<Identity> AthenticateBearer(
            const std::string& authorizationHeader) const = 0;
        virtual bool Require(Role required, const std::string& authorizationHeader,
                             Identity* out = nullptr) const = 0;
        virtual std::string IssueToken(const Identity& id, int ttlSeconds) const = 0;
    };

    void Set(std::shared_ptr<IAuthService> svr);
    std::shared_ptr<IAuthService> Get();

}  // namespace Auth