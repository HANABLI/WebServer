#pragma once
/**
 * @file Tenant.hpp
 * @brief This is the declaration of the FalcataIoTServer::Tenant class.
 * @copyright copyright © 2026 by Hatem Nabli.
 */

#include <Json/Json.hpp>
#include <Models/Core/CoreObject.hpp>
#include <Models/Core/IJsonSerialisable.hpp>
#include <memory>
#include <string>

namespace FalcataIoTServer
{
    class Tenant final : public CoreObject, public IJsonSerializable
    {
    public:
        ~Tenant() noexcept = default;
        Tenant(const Tenant&) = delete;
        Tenant(Tenant&&) noexcept = default;
        Tenant& operator=(const Tenant&) = delete;
        Tenant& operator=(Tenant&&) noexcept = default;

    public:
        explicit Tenant();

        const std::string& GetTenantSlug() const;
        void SetTenantSlug(const std::string& slug);

        const std::string& GetTenantName() const;
        void SetTenantName(const std::string& name);

        const std::vector<std::string> GetInsertParams() const override;
        const std::vector<std::string> GetUpdateParams() const override;
        const std::vector<std::string> GetRemoveParams() const override;
        const std::vector<std::string> GetDisableParams() const override;

        // JSON
        Json::Value ToJson() const override;
        void FromJson(const Json::Value& json) override;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}  // namespace FalcataIoTServer