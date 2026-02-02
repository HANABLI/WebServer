/**
 * @file AuthSigninPlugin.cpp
 * @brief
 * @copyright © 2026 by Hatem Nabli
 */

#include <Http/IServer.hpp>
#include <Managers/UserManager.hpp>
#include <Auth/AuthService.hpp>
#include <Auth/Guards.hpp>
#include <AuthService/AuthService.hpp>
#include <Models/Auth/User.hpp>
#include <Json/Json.hpp>
#include <StringUtils/StringUtils.hpp>
#include <PgClient/PgClient.hpp>
#include <WebServer/PluginEntryPoint.hpp>
#include <inttypes.h>
#include <functional>
#include <memory>

#ifdef _WIN32
#    define API __declspec(dllexport)
#else /* POSIX */
#    define API
#endif /* _WIN32 / POSIX */
namespace
{
    struct SpaceMapping
    {
        /**
         * This is the path to the resource space in the server.
         */
        std::vector<std::string> space;

        /**
         * This is the function to call to unregister the plugin as
         * handling this server resource space.
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

    struct AuthSigninPlugin
    {
        std::string pgConninfo;
        std::vector<SpaceMapping> spaces;
        std::shared_ptr<FalcataIoTServer::AuthServiceHs256> authSrv;
        std::shared_ptr<Postgresql::PgClient> pg;
        std::unique_ptr<FalcataIoTServer::UserManager> users;
        SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diag;
        std::shared_ptr<Http::Client::Response> JsonResponse(int code, const std::string& status,
                                                             const Json::Value& body) {
            auto r = std::make_shared<Http::Client::Response>();
            r->statusCode = code;
            r->status = status;
            r->headers.AddHeader("Content-Type", "application/json");
            r->headers.AddHeader("Access-Control-Allow-Origin", "*");
            r->headers.AddHeader("Access-Control-Allow-Headers",
                                 "Authorization, Content-Type, X-Tenant, X-Site");
            r->headers.AddHeader("Access-Control-Allow-Methods",
                                 "GET, POST, PATCH, DELETE, OPTIONS");
            r->body = body.ToEncoding();
            return r;
        }

        std::shared_ptr<Http::Client::Response> Error(int code, const char* msg) {
            Json::Value j(Json::Value::Type::Object);
            j.Set("error", msg);
            return JsonResponse(code,
                                (code == 401)   ? "Unauthorized"
                                : (code == 403) ? "Forbidden"
                                                : "Error",
                                j);
        }
    } authSigninPlugin;
}  // namespace

extern "C" API void LoadPlugin(Http::IServer* server, Json::Value configuration,
                               SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diag,
                               std::function<void()>& unloadDelegate) {
    unloadDelegate = [] {};

    authSigninPlugin.pg = std::make_shared<Postgresql::PgClient>();
    authSigninPlugin.authSrv = std::make_shared<FalcataIoTServer::AuthServiceHs256>(configuration);
    Auth::Set(authSigninPlugin.authSrv);
    authSigninPlugin.pgConninfo =
        StringUtils::ExpendEnvStringVar(std::string(configuration["PgConninfo"]).c_str());
    if (configuration.Has("spaces") &&
        (configuration["spaces"].GetType() == Json::Value::Type::Array))
    {
        const auto spaces = configuration["spaces"];
        for (size_t i = 0; i < spaces.GetSize(); ++i)
        {
            SpaceMapping spaceMapping;
            if (!ConfigureSpaceMapping(spaceMapping, spaces[i], diag))
            { return; }
            authSigninPlugin.spaces.push_back(spaceMapping);
        }
    } else
    {
        SpaceMapping spaceMapping;
        if (!ConfigureSpaceMapping(spaceMapping, configuration, diag))
        { return; }
        authSigninPlugin.spaces.push_back(spaceMapping);
    }
    authSigninPlugin.diag = diag;
    if (!authSigninPlugin.pg->Connect(authSigninPlugin.pgConninfo))
    {
        authSigninPlugin.diag("AuthSigninPlugin", 5,
                              StringUtils::sprintf("PG connect failed with: %s",
                                                   authSigninPlugin.pgConninfo.c_str()));
        return;
    }
    authSigninPlugin.users = std::make_unique<FalcataIoTServer::UserManager>(authSigninPlugin.pg);
    for (auto& space : authSigninPlugin.spaces)
    {
        const auto recousrcePath = space.space;
        space.unregisterationDelegate = server->RegisterResource(
            recousrcePath,
            [recousrcePath, configuration](const std::shared_ptr<Http::IServer::Request> request,
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

                const std::string last = recousrcePath.empty() ? "" : recousrcePath.back();

                try
                {
                    if (request->method == "POST" && last == "signin")
                    {
                        const auto body = Json::Value::FromEncoding(request->body);

                        const std::string tenantId = body["tenant_id"];
                        if (tenantId.empty())
                            return authSigninPlugin.Error(400, "tenant_id required");
                        const std::string username = body["user_name"];
                        const std::string password = body["password"];
                        if (username.empty() || password.empty())
                            return authSigninPlugin.Error(400, "username/password required");

                        auto user = authSigninPlugin.users->SigninCreateUser(body);
                        auto out = user->ToJson(true);
                        return authSigninPlugin.JsonResponse(200, "OK", out);
                    }
                    if (request->method == "POST" && last == "login")
                    {
                        const auto body = Json::Value::FromEncoding(request->body);
                        const std::string tenantId =
                            body.Has("tenant_id")
                                ? body["tenant_id"]
                                : Json::Value::FromEncoding(
                                      request->headers.GetHeaderValue("X-Tenant-Id"));
                        const std::string tenantSlug =
                            body.Has("tanant_slug")
                                ? body["tenant_slug"]
                                : Json::Value::FromEncoding(
                                      request->headers.GetHeaderValue("X-Tenant-Slug"));
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
                            authSigninPlugin.users->LoginVerify(tenantId, username, password, totp);

                        Auth::Identity id;
                        id.sub = user->GetUsername();
                        id.tenant_id = tenantId;
                        id.tenant_slug = tenantSlug;
                        id.role = Auth::ParseRole(user->GetRole());

                        const int ttl =
                            configuration.Has("JwtTtlSec") ? (int)configuration["JwtTtlSec"] : 3600;
                        const std::string token = authSigninPlugin.authSrv->IssueToken(id, ttl);

                        Json::Value out(Json::Value::Type::Object);
                        out.Set("access_token", token);
                        out.Set("token_type", "Bearer");
                        out.Set("role", user->GetRole());
                        out.Set("mfa_enabled", user->IsMfaEnabled());
                        return response;
                    }
                    if (last == "users")
                    {
                        auto resp = std::make_shared<Http::Client::Response>();
                        Auth::Identity id;
                        const std::string tenantSlug = request->headers.GetHeaderValue("X-Tenant");
                        if (!Auth::RequireTenantStrict(request, resp, tenantSlug, Auth::Role::Admin,
                                                       &id))
                        { return resp; }
                        const std::string tenantId =
                            id.tenant_id.empty() ? request->headers.GetHeaderValue("X-Tenant-Id")
                                                 : id.tenant_id;
                        if (tenantId.empty())
                        {
                            Json::Value err(Json::Value::Type::Object);
                            err.Set("error", "missing tenant_id");
                            return authSigninPlugin.JsonResponse(400, "Error", err);
                        }

                        if (request->method == "GET")
                        {
                            auto list = authSigninPlugin.users->ListUsers(tenantId, 200);
                            Json::Value arr(Json::Value::Type::Array);
                            for (const auto& u : list) arr.Add(u->ToJson());
                            return authSigninPlugin.JsonResponse(200, "OK", arr);
                        }

                        if (request->method == "POST")
                        {
                            const auto body = Json::Value::FromEncoding(request->body);
                            const std::string username = body["user_name"];
                            const std::string password = body["password"];
                            if (username.empty() || password.empty())
                            { return authSigninPlugin.Error(400, "username/password required"); }

                            auto user = authSigninPlugin.users->SigninCreateUser(body);
                            auto out = user->ToJson(true);
                            return authSigninPlugin.JsonResponse(200, "OK", out);
                        }
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
    unloadDelegate = []
    {
        for (const auto& space : authSigninPlugin.spaces)
        { space.unregisterationDelegate(); }
    };
    diag("AuthSigninPlugin", 0, "AuthSigninPlugin loaded successfully");
}

namespace
{
    PluginEntryPoint EntryPoint = &LoadPlugin;
}  // namespace
