#pragma once
/**
 * @file User.hpp
 * @brief This is the declaration of the user class.
 * @copyright © 2025 by Hatem Nabli.
 */
#include <PgClient/PgResult.hpp>
#include <Models/Core/CoreObject.hpp>
#include <Models/Core/IJsonSerialisable.hpp>

namespace FalcataIoTServer
{
    class User : public CoreObject, public IJsonSerializable
    {
    public:
        ~User();
        User(const User&) = delete;
        User(User&&) noexcept = default;
        User& operator=(const User&) = delete;
        User& operator=(User&&) noexcept = default;

    public:
        explicit User(/* args */);
        User(bool userEnabled, bool mfaEnabled);

        const std::string& GetTenantId() const;
        void SetTenantId(const std::string& v);

        const std::string& GetUsername() const;
        void SetUsername(const std::string& v);

        const std::string& GetEmail() const;
        void SetEmail(const std::string& v);

        const std::string& GetPasswordHash() const;
        void SetPasswordHash(const std::string& v);

        const std::string& GetRole() const;
        void SetRole(const std::string& v);

        bool IsEnabled() const;
        void SetEnabled(bool v);

        bool IsMfaEnabled() const;
        void SetMfaEnabled(bool v);

        const std::string& GetMfaSecretB32() const;
        void SetMfaSecretB32(const std::string& v);

        const int GetTotpDigits() const;
        void SetTotpDigits(int v);
        const int GetTotpPeriod() const;
        void SetTotpPeriod(int v);

        // RBAC by site: site_id -> role ("admin|operator|viewer")
        const Json::Value& GetSiteRoles() const;
        void SetSiteRoles(Json::Value v);
        void SetSiteRole(const std::string& siteId, const std::string& role);

        const std::vector<std::string> GetInsertParams() const override;
        const std::vector<std::string> GetUpdateParams() const override;
        const std::vector<std::string> GetRemoveParams() const override;
        const std::vector<std::string> GetDisableParams() const override;

        // JSON
        Json::Value ToJson() const override;
        Json::Value ToJson(bool includeSecrets) const;
        void FromJson(const Json::Value& json) override;
        static std::unique_ptr<User> FromRow(const Postgresql::PgResult& r, int row);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

}  // namespace FalcataIoTServer