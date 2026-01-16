/**
 * @file Guards.cpp
 * @brief this is the implementation of the authentication guard functions.
 * @copyright © copyright 2025 by Hatem Nabli.
 */

#include <Auth/Guards.hpp>
#include <Auth/Jwt.hpp>

namespace Auth
{
    void SetJsonError(std::shared_ptr<Http::Client::Response> r, int code, const char* msg) {
        r->statusCode = code;
        r->status = (code == 401)   ? "Unauthorized"
                    : (code == 403) ? "Forbidden"
                    : (code == 503) ? "Service Unavailable"
                                    : "Error";
        r->headers.AddHeader("Content-Type", "application/json");
        r->body = std::string(R"({"error":")") + msg + R"("})";
    }

    static bool HasSite(const Identity& id, const std::string& siteId) {
        if (id.site_ids.empty())
            return true;
        for (const auto& s : id.site_ids)
        {
            if (s == siteId)
                return true;
        }
        return false;
    }

    bool RequireRoleStrict(const std::shared_ptr<Http::IServer::Request> request,
                           std::shared_ptr<Http::Client::Response> response, Role required,
                           Identity* outIdentity) {
        auto svc = Get();
        if (!svc)
        {
            SetJsonError(response, 503, "auth service not available");
            return false;
        }

        const auto auth = request->headers.GetHeaderValue("Authorization");
        if (auth.empty())
        {
            response->headers.AddHeader("WWW-Authenticate", "Bearer");
            SetJsonError(response, 401, "missing Authorization");
            return false;
        }

        Identity id;
        if (!svc->Require(required, auth, &id))
        {
            auto opt = svc->AthenticateBearer(auth);
            if (!opt)
            {
                response->headers.AddHeader("WWW-Authenticate", "Bearer");
                SetJsonError(response, 401, "invalid token");
            } else
            { SetJsonError(response, 403, "insufficient role"); }
            return false;
        }

        if (outIdentity)
            *outIdentity = id;
        return true;
    }

    bool RequireTenantStrict(const std::shared_ptr<Http::IServer::Request> request,
                             std::shared_ptr<Http::Client::Response> response,
                             const std::string& tenantSlug, Role required, Identity* outIdentity) {
        Identity id;
        if (!RequireRoleStrict(request, response, required, &id))
            return false;
        if (!tenantSlug.empty() && !id.tenant_slug.empty() && id.tenant_slug != tenantSlug)
        {
            SetJsonError(response, 403, "tenant mismatch");
            return false;
        }

        if (outIdentity)
            *outIdentity = id;
        return true;
    }

    bool RequireTenantSiteStrict(const std::shared_ptr<Http::IServer::Request> request,
                                 std::shared_ptr<Http::Client::Response> response,
                                 const std::string& tenantSlug, const std::string& siteId,
                                 Role required, Identity* outIdentity) {
        Identity id;
        if (!RequireTenantStrict(request, response, tenantSlug, required, &id))
        { return false; }
        if (!siteId.empty() && !HasSite(id, siteId))
        {
            SetJsonError(response, 403, "site not allowed");
            return false;
        }

        if (outIdentity)
            *outIdentity = id;
        return true;
    }

}  // namespace Auth