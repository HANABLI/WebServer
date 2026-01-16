#pragma once
/**
 * @file Guards.hpp
 * @brief This is the declaration of the Auth::Guard functions.
 * @copyright  copyright © 2025 by Hatem Nabli.
 */
#include <Auth/AuthService.hpp>
#include <Auth/Role.hpp>
#include <Http/IServer.hpp>
#include <Http/Client.hpp>
#include <string>
#include <memory>

namespace Auth
{
    void SetJsonError(std::shared_ptr<Http::Client::Response> r, int code, const char* msg);

    bool RequireRoleStrict(const std::shared_ptr<Http::IServer::Request> request,
                           std::shared_ptr<Http::Client::Response> response, Role required,
                           Identity* outIdentity = nullptr);

    bool RequireTenantStrict(const std::shared_ptr<Http::IServer::Request> request,
                             std::shared_ptr<Http::Client::Response> response,
                             const std::string& tenantSlug, Role required,
                             Identity* outIdentity = nullptr);
    bool RequireTenantSiteStrict(const std::shared_ptr<Http::IServer::Request> request,
                                 std::shared_ptr<Http::Client::Response> response,
                                 const std::string& tenantSlug, const std::string& siteId,
                                 Role required, Identity* outIdentity = nullptr);

}  // namespace Auth