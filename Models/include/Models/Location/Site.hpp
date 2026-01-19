/**
 * @file Site.hpp
 */

#pragma once
#include <Models/Core/CoreObject.hpp>
#include <Models/Core/IJsonSerialisable.hpp>
#include <memory>

namespace FalcataIoTServer
{
    class Site final : public CoreObject, public IJsonSerializable
    {
    public:
        ~Site() noexcept;
        Site(const Site&) = delete;
        Site(Site&&) noexcept = default;
        Site& operator=(const Site&) = delete;
        Site& operator=(Site&&) noexcept = default;

    public:
        explicit Site(/* args */);
        Site(const Json::Value&);

        // Champs BDD
        const std::string& GetName() const;
        void SetName(const std::string& v);

        const std::string& GetDescription() const;
        void SetDescription(const std::string& v);
        const std::string& GetKind() const;  // "farm" | "city" | "station" | "tunnel" | ...
        void SetKind(const std::string& v);

        const std::string& GetCountry() const;
        void SetCountry(const std::string& v);

        const std::string& GetTimezone() const;
        void SetTimezone(const std::string& v);

        const std::vector<std::string>& GetTags() const;
        void SetTags(std::vector<std::string> v);
        void AddTag(const std::string& tag);

        const Json::Value& GetMetadata() const;
        void SetMetadata(const Json::Value& v);

        const std::string& GetCreatedAt() const;
        void SetCreatedAt(const std::string& v);

        const std::string& GetUpdatedAt() const;
        void SetUpdatedAt(const std::string& v);

        // Champs "transient" (non persistés) pour matérialiser la hiérarchie
        const std::vector<std::string>& GetZoneIds() const;
        void SetZoneIds(std::vector<std::string> ids);
        void AddZoneId(const std::string& zoneId);

        const std::vector<std::string> GetInsertParams() const override;
        const std::vector<std::string> GetUpdateParams() const override;
        const std::vector<std::string> GetRemoveParams() const override;
        const std::vector<std::string> GetDisableParams() const override;

        // IJsonSerializable
        Json::Value ToJson() const override;
        void FromJson(const Json::Value& json) override;

    private:
        struct Impl;

        std::unique_ptr<Impl> impl_;
    };

}  // namespace FalcataIoTServer