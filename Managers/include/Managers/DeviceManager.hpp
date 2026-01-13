#pragma once

/**
 * @file DeviceManager.hpp
 * @brief This is the declaration of the FalcataIoTServer::DeviceManager class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */

#include <Managers/DeviceRegistry.hpp>
#include <Managers/MqttDeviceConnector.hpp>
#include <MqttV5/MqttClient.hpp>
#include <MqttV5/MqttV5Properties.hpp>
#include <MqttV5/MqttV5Types.hpp>
#include <Repositories/GenericRepo.hpp>
#include <Repositories/ServerRepo.hpp>
#include <Repositories/IotDevicesRepo.hpp>
#include <Repositories/MqttTopicRepo.hpp>
#include <PgClient/PgClient.hpp>

#include <memory>
#include <unordered_map>
namespace FalcataIoTServer
{
    class DeviceManager
    {
    public:
        explicit DeviceManager(std::shared_ptr<Postgresql::PgClient> pg,
                               std::shared_ptr<MqttV5::MqttClient> client);

        // Load from DB
        void ReloadAll();

        DeviceRegistry& Registry();

        const DeviceRegistry& Registry() const;
        //
        void SyncAllMqttDevices();

        std::shared_ptr<MqttV5::MqttClient::Transaction> PublishToBroker(
            const std::string& serverId, const std::string& topic, const std::string& payload,
            const bool retain, MqttV5::QoSDelivery qos, const uint16_t packetID,
            MqttV5::Properties* properties);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}  // namespace FalcataIoTServer
