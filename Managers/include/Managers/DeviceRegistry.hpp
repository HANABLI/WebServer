#pragma once
/**
 * @file DeviceRegistry.hpp
 * @brief This module contains the declaration of the FalcataIoTServer::DeviceRegistry
 * class.
 * @copyright © 2025 by Hatem Nabli.
 */
#include <Models/Location/Site.hpp>
#include <Models/Location/Zone.hpp>
#include <Models/Core/Server.hpp>
#include <Models/Core/IoTDevice.hpp>
#include <Models/IoTDevices/MqttDevice.hpp>
#include <Models/Data/MqttTopic.hpp>
#include <memory>
#include <unordered_map>

namespace FalcataIoTServer
{
    class DeviceRegistry
    {
    public:
        void Clear();

        void UpsertSite(std::shared_ptr<Site> Site);
        std::shared_ptr<Site> GetSite(const std::string& id) const;
        std::vector<std::shared_ptr<Site>> GetAllSites() const;

        void UpsertZone(std::shared_ptr<Zone> zone);
        std::shared_ptr<Zone> GetZone(const std::string& id) const;
        std::vector<std::shared_ptr<Zone>> GetAllZones() const;

        void UpsertServer(std::shared_ptr<Server> server);
        std::shared_ptr<Server> GetServer(const std::string& id) const;
        std::vector<std::shared_ptr<Server>> GetAllServers() const;

        void UpsertDevice(std::shared_ptr<IoTDevice> server);
        std::shared_ptr<IoTDevice> GetDevice(const std::string& id) const;
        std::vector<std::shared_ptr<IoTDevice>> GetAllDevices() const;

        void SetTopicsForDevice(const std::string& deviceId,
                                std::vector<std::shared_ptr<MqttTopic>> topics);
        std::vector<std::shared_ptr<MqttTopic>> GetTopicsForDevice(
            const std::string& deviceId) const;

        std::vector<std::shared_ptr<MqttDevice>> GetAllMqttDevices() const;

    private:
        std::unordered_map<std::string, std::shared_ptr<Site>> sites_;
        std::unordered_map<std::string, std::shared_ptr<Zone>> zones_;
        std::unordered_map<std::string, std::shared_ptr<Server>> servers_;
        std::unordered_map<std::string, std::shared_ptr<IoTDevice>> devices_;
        std::unordered_map<std::string, std::vector<std::shared_ptr<MqttTopic>>> topics_;
    };
}  // namespace FalcataIoTServer
