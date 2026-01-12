#pragma once
/**
 * @file MqttDevice.hpp
 */
#include <Models/Core/IoTDevice.hpp>
#include <Models/Data/MqttTopic.hpp>
#include <memory>
#include <string>
#include <set>
namespace FalcataIoTServer
{
    class MqttDevice final : public IoTDevice
    {
        // Life cycle managment
    public:
        ~MqttDevice() noexcept = default;
        MqttDevice(const MqttDevice&) = delete;
        MqttDevice(MqttDevice&&) noexcept = default;
        MqttDevice& operator=(const MqttDevice&) = delete;
        MqttDevice& operator=(MqttDevice&&) noexcept = default;
        // Methods
    public:
        explicit MqttDevice();
        MqttDevice(std::string id, std::string serverId, std::string name, std::string kind,
                   std::string protocol, bool enabled, std::string zoneId);

        const std::set<std::shared_ptr<MqttTopic>> GetTopics() const;
        void SetTopics(std::set<std::shared_ptr<MqttTopic>>& topics);
        void AddTopic(std::shared_ptr<MqttTopic>& topicId);
        void DeleteTopic(std::shared_ptr<MqttTopic>& topicId);

        // Sérialisation
        Json::Value ToJson() const override;

        void FromJson(const Json::Value& json) override;

    private:
        struct Impl;
        std::unique_ptr<Impl> mqttDevice_;
    };
}  // namespace FalcataIoTServer