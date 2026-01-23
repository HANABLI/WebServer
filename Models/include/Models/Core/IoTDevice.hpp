#pragma once
/**
 * @file IoTDevice.hpp
 * @brief This is the declaration of the IoTDevice base class that
 * heritate both CoreObject and Device CRTP classes.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#include <Models/Core/CoreObject.hpp>
#include <Models/Core/Device.hpp>
#include <Models/Location/Zone.hpp>
#include <vector>
#include <string>

namespace FalcataIoTServer
{
    class IoTDevice : public CoreObject, public Device<IoTDevice>
    {
        // Life cycle managment
    public:
        virtual ~IoTDevice() noexcept = default;
        IoTDevice(const IoTDevice&) = delete;
        IoTDevice(IoTDevice&&) noexcept = default;
        IoTDevice& operator=(const IoTDevice&) = delete;
        IoTDevice& operator=(IoTDevice&&) noexcept = default;

    public:
        // Methods
        explicit IoTDevice();
        IoTDevice(const Json::Value&);
        IoTDevice(std::string id, std::string serverId, std::string name, std::string kind,
                  std::string protocol, bool enabled, std::string zoneId);
        IoTDevice(std::string name, std::string kind, std::string protocol, bool enabled,
                  std::string zoneId);

        const std::string GetServerId() const;
        void SetServerId(const std::string& serverId);

        const std::string GetZone() const;
        void SetZone(const std::string& zoneId);

        const std::vector<std::string> GetEvents() const;
        void SetEvents(const std::vector<std::string>& eventIds);
        void AddEvent(const std::string& eventId);

        const std::string& GetSiteId() const;
        void SetSiteId(const std::string& v);

        const std::string& GetTypeId() const;
        void SetTypeId(const std::string& v);

        const std::string& GetExternalId() const;
        void SetExternalId(const std::string& v);

        const std::string& GetLastSeenAt() const;
        void SetLastSeenAt(const std::string& v);

        const std::vector<std::string>& GetTags() const;
        void SetTags(std::vector<std::string> v);
        void AddTag(const std::string& tag);

        const Json::Value& GetMetadata() const;
        void SetMetadata(const Json::Value& v);

        const std::string& GetCreatedAt() const;
        void SetCreatedAt(const std::string& v);

        const std::string& GetUpdatedAt() const;
        void SetUpdatedAt(const std::string& v);

        virtual const std::vector<std::string> GetInsertParams() const override;
        virtual const std::vector<std::string> GetUpdateParams() const override;
        virtual const std::vector<std::string> GetRemoveParams() const override;
        virtual const std::vector<std::string> GetDisableParams() const override;

        // Sérialisation générique d’un "Server"
        virtual Json::Value ToJson() const override;

        virtual void FromJson(const Json::Value& json) override;

    public:
        struct Impl
        {
            std::string siteId;
            std::string zoneId;
            std::string typeId;

            std::string serverId;
            std::string externalId;
            std::string lastSeenAt;

            std::vector<std::string> tags;
            Json::Value metadata = Json::Value(Json::Value::Type::Object);

            std::string createdAt;
            std::string updatedAt;

            // existing
            std::string name;
            std::string kind;
            std::string protocol;
            bool enabled = true;
            std::vector<std::string> events;
        };

        std::unique_ptr<Impl> impl_;
    };

}  // namespace FalcataIoTServer