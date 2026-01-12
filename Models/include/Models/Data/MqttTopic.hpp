#pragma once

/**
 * @file mqttTopic.hpp
 */
#include <Json/Json.hpp>
#include <MqttV5/MqttV5Constants.hpp>
#include <MqttV5/MqttV5Packets.hpp>
#include <MqttV5/MqttV5Types.hpp>
#include <Models/Core/CoreObject.hpp>
#include <Models/Core/IJsonSerialisable.hpp>

#include <string>
#include <memory>
namespace FalcataIoTServer
{
    class MqttTopic final : public CoreObject, public IJsonSerializable
    {
        // Life cycle managment
    public:
        ~MqttTopic() noexcept = default;
        MqttTopic(const MqttTopic&) = delete;
        MqttTopic(MqttTopic&&) noexcept = default;
        MqttTopic& operator=(const MqttTopic&) = delete;
        MqttTopic& operator=(MqttTopic&&) noexcept = default;
        // Methods
    public:
        explicit MqttTopic();
        MqttTopic(const Json::Value&);

        std::shared_ptr<MqttV5::SubscribeTopic> BuildTopic() const;
        std::shared_ptr<MqttV5::UnsubscribeTopic> BuildUnsubTopic() const;

        const std::string GetId() const;
        void SetId(const std::string& id);

        const std::string GetDeviceId() const;
        void SetDeviceId(const std::string& id);

        const std::string GetRole() const;  // telemetry|command|state|event|config
        void SetRole(const std::string& role);

        const std::string GetTopic() const;  //{tenant}/{site}/{zone}/{device}/{role}
        void SetTopic(const std::string& topic);

        MqttV5::QoSDelivery GetQoS() const;
        void SetQoS(MqttV5::QoSDelivery qos);

        MqttV5::RetainHandling GetRetain() const;
        void SetRetain(MqttV5::RetainHandling retain);

        bool WithAutoFeedBack() const;
        void SetWithAutoFeedBAck(bool v);

        bool GetRetainAsPublished() const;
        void SetRetainAsPublished(bool retainAsPublished);

        const std::string& GetDirection() const;  // pub|sub|pubsub
        void SetDirection(const std::string& direction);

        bool IsEnabled() const;
        void SetEnabled(bool v);

        const Json::Value& GetMetaData() const;
        void SetMetaData(const Json::Value& v);
        const std::string& GetCreatedAt() const;
        void SetCreatedAt(const std::string& v);
        const std::string& GetUpdatedAt() const;
        void SetUpdatedAt(const std::string& v);

        const std::vector<std::string> GetInsertParams() const override;
        const std::vector<std::string> GetUpdateParams() const override;
        const std::vector<std::string> GetRemoveParams() const override;
        const std::vector<std::string> GetDisableParams() const override;

        Json::Value ToJson() const override;
        void FromJson(const Json::Value& json) override;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}  // namespace FalcataIoTServer