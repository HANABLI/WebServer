#pragma once
/**
 * @file MqttDeviceConnector.hpp
 * @brief This is the declaration of the FalcataIoTServer::MqttDeviceConnector class.
 * @copyright copyright © 2025 by Hatem Nabli.
 */

#include <Models/IoTDevices/MqttDevice.hpp>
#include <Models/Servers/MqttBroker.hpp>
#include <Models/Data/MqttTopic.hpp>
#include <MqttV5/MqttClient.hpp>

#include <memory>
#include <unordered_set>
#include <string>

namespace FalcataIoTServer
{
    class MqttDeviceConnector
    {
    public:
        explicit MqttDeviceConnector(std::shared_ptr<MqttV5::MqttClient> client,
                                     std::shared_ptr<MqttBroker> broker);
        ~MqttDeviceConnector() noexcept;

        // Synchronize the state of topic (subscribed)
        void SyncDevice(const MqttDevice& device);

        // Unsubscribe topics
        void UnsyncDevice(const MqttDevice& device);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}  // namespace FalcataIoTServer