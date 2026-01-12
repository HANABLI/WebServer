#pragma once
/**
 * @file Zone.hpp
 */
#include <Json/Json.hpp>
#include <Models/Core/CoreObject.hpp>
#include <Models/Core/IJsonSerialisable.hpp>
#include <memory>
namespace FalcataIoTServer
{
    class Zone final : public CoreObject, public IJsonSerializable
    {
        // Life cycle managment
    public:
        ~Zone() noexcept = default;
        Zone(const Zone&) = delete;
        Zone(Zone&&) noexcept = default;
        Zone& operator=(const Zone&) = delete;
        Zone& operator=(Zone&&) noexcept = default;

        // Methods
    public:
        explicit Zone();
        Zone(const Json::Value&);

        const std::string& GetSiteId() const;
        void SetSiteId(const std::string& v);

        const std::string& GetName() const;
        void SetName(const std::string& v);

        const std::string& GetDescription() const;
        void SetDescription(const std::string& v);

        const std::string& GetKind() const;
        void SetKind(const std::string& v);

        const Json::Value& GetGeoJson() const;
        void SetGeoJson(const Json::Value& v);

        const std::vector<std::string>& GetTags() const;
        void SetTags(std::vector<std::string> v);
        void AddTag(const std::string& tag);

        const Json::Value& GetMetadata() const;
        void SetMetadata(const Json::Value& v);

        const std::string& GetCreatedAt() const;
        void SetCreatedAt(const std::string& v);

        const std::string& GetUpdatedAt() const;
        void SetUpdatedAt(const std::string& v);

        // Champs "transient" : devices de la zone (ids)
        const std::vector<std::string>& GetDeviceIds() const;
        void SetDeviceIds(std::vector<std::string> ids);
        void AddDeviceId(const std::string& deviceId);

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