#include <AuthService/AuthService.hpp>
#include <Auth/Jwt.hpp>

namespace FalcataIoTServer
{
    struct AuthServiceHs256::Impl
    {
        Json::Value cfg;

        std::string jwtSecret, jwtIss, jwtAud;

        Impl(Json::Value cfg) : cfg(cfg) {
            jwtAud = cfg.Has("JwtAud") ? cfg["JwtAud"] : "";
            jwtIss = cfg.Has("JwtIss") ? cfg["JwtIss"] : "";
            jwtSecret = cfg.Has("JwtSecret") ? cfg["JwtSecret"] : "";
        };
        ~Impl() noexcept = default;
    };

    AuthServiceHs256::AuthServiceHs256(Json::Value cfg) :
        impl_(std::make_unique<Impl>(std::move(cfg))) {}

    AuthServiceHs256::~AuthServiceHs256() noexcept = default;

    std::optional<Auth::Identity> AuthServiceHs256::AthenticateBearer(
        const std::string& authorizationHeader) const {
        const std::string prefix = "Bearer ";
        if (authorizationHeader.rfind(prefix, 0) != 0)
            return std::nullopt;
        const std::string token = authorizationHeader.substr(prefix.size());
        if (token.empty() || impl_->jwtSecret.empty())
            return std::nullopt;

        try
        {
            const auto verified =
                Auth::VerifyHs256(token, impl_->jwtSecret, impl_->jwtIss, impl_->jwtAud);
            Auth::Identity id;
            id.claims = verified.payload;
            id.sub = verified.payload.Has("sub") ? verified.payload["sub"] : "";
            id.tenant_slug =
                verified.payload.Has("tenant_slug") ? verified.payload["tenant_slug"] : "";
            id.tenant_id = verified.payload.Has("tenant_id") ? verified.payload["tenant_id"] : "";
            id.role = verified.payload.Has("role") ? Auth::ParseRole(verified.payload["role"])
                                                   : Auth::Role::Viewer;
            if (verified.payload.Has("site_ids") &&
                verified.payload["site_ids"].GetType() == Json::Value::Type::Array)
            {
                const auto arr = verified.payload["site_ids"];
                for (size_t i = 0; i < arr.GetSize(); ++i)
                { id.site_ids.push_back(arr[i]); }
            }
            return id;
        }
        catch (...)
        { return std::nullopt; }
    }

    bool AuthServiceHs256::Require(Auth::Role required, const std::string& authorizationHeader,
                                   Auth::Identity* out) const {
        auto id = AthenticateBearer(authorizationHeader);
        if (!id)
            return false;
        if (!Auth::HasAtLeast(id->role, required))
            return false;
        if (out)
            *out = *id;
        return true;
    }

    std::string AuthServiceHs256::IssueToken(const Auth::Identity& id, int ttlSeconds) const {
        Json::Value header(Json::Value::Type::Object);
        header.Set("typ", "JWT");
        header.Set("alg", "HS256");

        const long now = Auth::NowEpoch();
        Json::Value payload(Json::Value::Type::Object);
        payload.Set("sub", id.sub);
        payload.Set("role", Auth::ToString(id.role));
        payload.Set("tenant_slug", id.tenant_slug);
        payload.Set("tenant_id", id.tenant_id);
        payload.Set("iat", (int)now);
        payload.Set("nbf", (int)now);
        payload.Set("exp", (int)(now + ttlSeconds));
        if (!impl_->jwtIss.empty())
            payload.Set("iss", impl_->jwtIss);
        if (!impl_->jwtAud.empty())
            payload.Set("aud", impl_->jwtAud);

        Json::Value sites(Json::Value::Type::Array);
        for (const auto& s : id.site_ids)
        { sites.Add(s); }
        payload.Set("sites_ids", sites);
        return Auth::MakeHs256(header, payload, impl_->jwtSecret);
    }
}  // namespace FalcataIoTServer