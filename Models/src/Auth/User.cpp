/**
 * @file User.cpp
 * @brief thi is the implementation of authentication user class.
 * @copyright © 2025 by Hatem Nabli.
 */

#include <Models/Auth/User.hpp>
#include <Auth/Password.hpp>
#include <Auth/Totp.hpp>
#include <PgClient/PgResult.hpp>

namespace FalcataIoTServer
{
    struct User::Impl
    {
        std::string tenantId;
        std::string userName;
        std::string email;
        bool disabled = false;

        std::string passwordHash;

        bool mfaEnabled = false;
        std::string totpSecretB32;
        int totpDigits = 6;
        int totpPeriod = 30;

        std::string role;
        Json::Value siteRoles = Json::Value(Json::Value::Type::Object);

        std::string createdAt;
        std::string updatedAt;
    };

    User::User() : CoreObject(), impl_(std::make_unique<Impl>()) {}
    User::User(bool userEnabled, bool mfaEnabled) : User() {
        impl_->mfaEnabled = mfaEnabled;
        impl_->disabled = !userEnabled;
    }
    User::~User() noexcept = default;

    const std::string& User::GetTenantId() const { return impl_->tenantId; }
    void User::SetTenantId(const std::string& v) { impl_->tenantId = v; }

    const std::string& User::GetUsername() const { return impl_->userName; }
    void User::SetUsername(const std::string& v) { impl_->userName = v; }

    const std::string& User::GetEmail() const { return impl_->email; }
    void User::SetEmail(const std::string& v) { impl_->email = v; }

    const std::string& User::GetPasswordHash() const { return impl_->passwordHash; }

    void User::SetPasswordHash(const std::string& v) { impl_->passwordHash = v; }

    const std::string& User::GetRole() const { return impl_->role; }
    void User::SetRole(const std::string& v) { impl_->role = v; }

    bool User::IsEnabled() const { return !impl_->disabled; }
    void User::SetEnabled(bool v) { impl_->disabled = v; }

    bool User::IsMfaEnabled() const { return impl_->mfaEnabled; }
    void User::SetMfaEnabled(bool v) { impl_->mfaEnabled = v; }

    const std::string& User::GetMfaSecretB32() const { return impl_->totpSecretB32; }
    void User::SetMfaSecretB32(const std::string& v) { impl_->totpSecretB32 = v; }

    const int User::GetTotpDigits() const { return impl_->totpDigits; }
    void User::SetTotpDigits(int v) { impl_->totpDigits = v; }

    const int User::GetTotpPeriod() const { return impl_->totpDigits; }
    void User::SetTotpPeriod(int v) { impl_->totpDigits = v; }

    // RBAC by site: site_id -> role ("admin|operator|viewer")
    const Json::Value& User::GetSiteRoles() const { return impl_->siteRoles; }
    void User::SetSiteRoles(Json::Value v) { impl_->siteRoles = v; }
    void User::SetSiteRole(const std::string& siteId, const std::string& role) {
        impl_->siteRoles.Set(siteId, role);
    }

    const std::vector<std::string> User::GetInsertParams() const {
        std::vector<std::string> out;
        out.push_back(Uuid_s());
        out.push_back(impl_->tenantId);
        out.push_back(impl_->userName);
        out.push_back(impl_->email);
        out.push_back(impl_->passwordHash);
        out.push_back(impl_->mfaEnabled ? "true" : "false");
        out.push_back(impl_->totpSecretB32);
        out.push_back(std::to_string(impl_->totpDigits));
        out.push_back(std::to_string(impl_->totpPeriod));
        out.push_back(impl_->role);
        out.push_back(impl_->disabled ? "true" : "false");
        out.push_back(impl_->siteRoles.ToEncoding());
        return out;
    }

    const std::vector<std::string> User::GetUpdateParams() const {
        std::vector<std::string> out;
        out.push_back(impl_->tenantId);
        out.push_back(Uuid_s());
        out.push_back(impl_->email);
        out.push_back(impl_->role);
        out.push_back(impl_->disabled ? "true" : "false");
        out.push_back(impl_->mfaEnabled ? "true" : "false");
        out.push_back(std::to_string(impl_->totpDigits));
        out.push_back(std::to_string(impl_->totpPeriod));
        out.push_back(impl_->siteRoles.ToEncoding());
        return out;
    }

    const std::vector<std::string> User::GetRemoveParams() const {
        std::vector<std::string> out;
        out.push_back(impl_->tenantId);
        out.push_back(Uuid_s());
        return out;
    }

    const std::vector<std::string> User::GetDisableParams() const {
        std::vector<std::string> out;
        out.push_back(impl_->tenantId);
        out.push_back(Uuid_s());
        out.push_back(impl_->disabled ? "true" : "false");
        return out;
    }

    // JSON
    Json::Value User::ToJson() const {
        Json::Value j(Json::Value::Type::Object);
        j.Set("id", Uuid_s());
        j.Set("tenant_id", impl_->tenantId);
        j.Set("user_name", impl_->userName);
        j.Set("email", impl_->email);
        j.Set("disabled", impl_->disabled);
        j.Set("mfa_enabled", impl_->mfaEnabled);
        j.Set("role", impl_->role);
        j.Set("site_roles", impl_->siteRoles);
        return j;
    }

    Json::Value User::ToJson(bool includeSecrets) const {
        auto j = ToJson();
        if (includeSecrets && impl_->mfaEnabled)
        {
            j.Set("totp_digits", impl_->totpDigits);
            j.Set("totp_period", impl_->totpPeriod);
            j.Set("totp_secret_b32", impl_->totpSecretB32);
        }
        return j;
    }

    void User::FromJson(const Json::Value& json) {
        if (json.Has("id"))
            UuidFromString(json["id"]);
        if (json.Has("tenant_id"))
            SetTenantId(json["tenant_id"]);
        if (json.Has("user_name"))
            SetUsername(json["user_name"]);
        if (json.Has("email"))
            SetEmail(json["email"]);
        if (json.Has("disabled"))
            SetEnabled(static_cast<bool>(json["disabled"]));
        if (json.Has("mfa_enabled"))
            SetMfaEnabled(static_cast<bool>(json["mfa_enabled"]));
        if (json.Has("totp_digits"))
            SetTotpDigits((int)json["totp_digits"]);
        if (json.Has("totp_period"))
            SetTotpPeriod((int)json["totp_period"]);
        if (json.Has("role"))
            SetRole(json["role"]);
        if (json.Has("password_hash"))
            SetPasswordHash(json["password_hash"]);
        if (json.Has("password"))
            SetPasswordHash(Auth::HashPasswordArgon2id(json["password"]));
        if (json.Has("totp_secret_b32"))
            SetMfaSecretB32(json["totp_secret_b32"]);
        else if (impl_->mfaEnabled)
        { SetMfaSecretB32(Auth::TotpGenerateSecretBase32(20)); }
        if (json.Has("site_roles"))
        {
            auto j = json["site_roles"];
            for (auto& i = j.begin(); i != j.end(); ++i)
            { SetSiteRole(i.key(), i.value()); }
        }
    }

    std::unique_ptr<User> User::FromRow(const Postgresql::PgResult& r, int row) {
        auto u = std::make_unique<User>();
        u->UuidFromString(r.Text(row, "id"));
        u->SetTenantId(r.Text(row, "tenant_id"));
        u->SetUsername(r.Text(row, "user_name"));
        u->SetEmail(r.Text(row, "email"));
        u->SetEnabled(r.Bool(row, "disabled", true));
        u->SetRole(r.Text(row, "role"));
        u->SetMfaEnabled(r.Bool(row, "mfa_enabled", false));
        u->SetMfaSecretB32(r.Text(row, "totp_secret_b32"));
        u->SetPasswordHash(r.Text(row, "password_hash"));
        u->SetTotpPeriod(r.Int(row, "totp_period", 30));
        u->SetTotpDigits(r.Int(row, "totp_digits", 6));
        auto j = r.Json(row, "site_roles", Json::Value::Type::Object);
        if (j.ToEncoding() != "{}" && j.ToEncoding() != "null")
        {
            for (auto i = j.begin(); i != j.end(); ++i)
            {
                u->SetSiteRole(i.key(), i.value());
            }
        } 
        return u;
    }

}  // namespace FalcataIoTServer
