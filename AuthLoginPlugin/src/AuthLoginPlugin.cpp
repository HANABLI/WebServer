/**
 * @file AuthLoginPlugin.cpp
 * @brief This represent the implementation of the AuthLoginPlugin.
 * @copyright © 2026 by Hatem Nabli
 */

#include <Http/IServer.hpp>
#include <Json/Json.hpp>
#include <Auth/Totp.hpp>
#include <Auth/Jwt.hpp>
#include <Auth/Guards.hpp>
#include <Managers/UserManager.hpp>
#include <AuthService/AuthService.hpp>
#include <SystemUtils/DiagnosticsSender.hpp>
#include <SystemUtils/CryptoRandom.hpp>
#include <WebServer/PluginEntryPoint.hpp>
#include <PgClient/PgClient.hpp>

#ifdef _WIN32
#    define API __declspec(dllexport)
#else /*POSIX*/
#    define API
#endif /* _WIN32 / POSIX*/
namespace
{
    struct SpaceMapping
    {
        /**
         * This is the path to the resource space in the server.
         */
        std::vector<std::string> space;

        /**
         * This is the function to call to unregister the plugin.
         */
        Http::IServer::UnregistrationDelegate unregisterationDelegate;
    };

    bool ConfigureSpaceMapping(
        SpaceMapping& spaceMapping, Json::Value configuration,
        SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessageDelegate) {
        Uri::Uri uri;
        if (!configuration.Has("space"))
        {
            diagnosticMessageDelegate("AuthSigninPlugin", SystemUtils::DiagnosticsSender::ERROR,
                                      "no 'space' URI in configuration");
            return false;
        }
        if (!uri.ParseFromString(configuration["space"]))
        {
            diagnosticMessageDelegate("AuthSigninPlugin", SystemUtils::DiagnosticsSender::ERROR,
                                      "unable to parse 'space' uri in configuration");
            return false;
        }
        spaceMapping.space = uri.GetPath();
        spaceMapping.space.erase(spaceMapping.space.begin());
        return true;
    }

    struct AuthLoginPlugin
    {
        std::string pgConninfo;
        std::vector<SpaceMapping> spaces;
        std::shared_ptr<FalcataIoTServer::AuthServiceHs256> authSrv;
        std::shared_ptr<Postgresql::PgClient> pg;
        std::unique_ptr<FalcataIoTServer::UserManager> users;
        SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diag;
    } authLoginPlugin;

}  // namespace
extern "C" API void LoadPlugin(Http::IServer* server, Json::Value configuration,
                               SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diag,
                               std::function<void()>& unloadDelegate) {
    authLoginPlugin.pg = std::make_shared<Postgresql::PgClient>();
    authLoginPlugin.authSrv = std::make_shared<FalcataIoTServer::AuthServiceHs256>(configuration);
    Auth::Set(authLoginPlugin.authSrv);
    authLoginPlugin.pgConninfo = configuration["PgConninfo"];
    if (configuration.Has("Spaces") &&
        configuration["Spaces"].GetType() == Json::Value::Type::Array)
    {
        const auto spaces = configuration["Spaces"];
        for (size_t i = 0; i < spaces.GetSize(); ++i)
        {
            SpaceMapping spaceMapping;
            if (!ConfigureSpaceMapping(spaceMapping, spaces[i], diag))
            { return; }
            authLoginPlugin.spaces.push_back(spaceMapping);
        }
    } else
    {
        SpaceMapping spaceMapping;
        if (!ConfigureSpaceMapping(spaceMapping, configuration, diag))
        { return; }
        authLoginPlugin.spaces.push_back(spaceMapping);
    }
    authLoginPlugin.diag = diag;
    if (!authLoginPlugin.pg->Connect(authLoginPlugin.pgConninfo))
    {
        authLoginPlugin.diag("AuthSigninPlugin", 5, "PG connect failed");
        return;
    }
    authLoginPlugin.users = std::make_unique<FalcataIoTServer::UserManager>(authLoginPlugin.pg);
    for (auto& space : authLoginPlugin.spaces)
    {
        const auto resourcePath = space.space;
        space.unregisterationDelegate = server->RegisterResource(
            resourcePath,
            [resourcePath, configuration](const std::shared_ptr<Http::IServer::Request> request,
                                          std::shared_ptr<Http::Connection> connection,
                                          const std::string& trailer)
            {
                auto response = std::make_shared<Http::Client::Response>();
                response->headers.AddHeader("Content-Type", "application/json");
                response->headers.AddHeader("Access-Control-Allow-Origin", "*");
                response->headers.AddHeader(
                    "Access-Control-Allow-Headers",
                    "Authorization, Content-Type, X-Tenant, X-Tenant-Id, X-Site");
                response->headers.AddHeader("Access-Control-Allow-Methods",
                                            "GET, POST, PATCH, DELETE, OPTIONS");
                if (request->method == "OPTIONS")
                {
                    response->statusCode = 204;
                    response->status = "No Content";
                    return response;
                }
                const std::string last = resourcePath.empty() ? "" : resourcePath.back();
                try
                {
                    if (request->method == "POST" && last == "login")
                    {
                        const auto body = Json::Value::FromEncoding(request->body);
                        const std::string tenantId =
                            body.Has("tenant_id") ? body["tenant_id"]
                                                  : request->headers.GetHeaderValue("X-Tenant-Id");
                        const std::string tenantSlug =
                            body.Has("tanant_slug")
                                ? body["tenant_slug"]
                                : request->headers.GetHeaderValue("X-Tenant-Slug");
                        const std::string username = body["user_name"];
                        const std::string password = body["password"];
                        const std::string totp = body.Has("totp") ? body["totp"] : "";
                        if (tenantId.empty())
                        {
                            Auth::SetJsonError(response, 400, "tenant-id required");
                            return response;
                        }
                        if (username.empty() || password.empty())
                        {
                            Auth::SetJsonError(response, 400, "username/password required");
                            return response;
                        }
                        auto user =
                            authLoginPlugin.users->LoginVerify(tenantId, username, password, totp);

                        Auth::Identity id;
                        id.sub = user->GetUsername();
                        id.tenant_id = tenantId;
                        id.tenant_slug = tenantSlug;
                        id.role = Auth::ParseRole(user->GetRole());

                        const int ttl =
                            configuration.Has("JwtTtlSec") ? (int)configuration["JwtTtlSec"] : 3600;
                        const std::string token = authLoginPlugin.authSrv->IssueToken(id, ttl);

                        Json::Value out(Json::Value::Type::Object);
                        out.Set("access_token", token);
                        out.Set("token_type", "Bearer");
                        out.Set("role", user->GetRole());
                        out.Set("mfa_enabled", user->IsMfaEnabled());
                        return response;
                    }
                    response->statusCode = 404;
                    response->status = "Not Found";
                    response->body = R"({"error":"unknown route"})";
                    return response;
                }
                catch (const std::exception& e)
                {
                    response->statusCode = 500;
                    response->status = "Internal Server Error";
                    Json::Value err(Json::Value::Type::Object);
                    err.Set("error", e.what());
                    response->body = err.ToEncoding();
                    return response;
                }
            });
    }
}

namespace
{
    PluginEntryPoint EntryPoint = &LoadPlugin;
}
