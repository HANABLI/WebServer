/**
 * @file DeviceRegistry.cpp
 * @brief This is the implementation of the FalcataIoTServer::DeviceRegistry class
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#include <Managers/DeviceRegistry.hpp>
namespace FalcataIoTServer
{
    void DeviceRegistry::Clear() {
        servers_.clear();
        devices_.clear();
        topics_.clear();
    }
    // TODO Deal with sites and zones
    void DeviceRegistry::UpsertServer(std::shared_ptr<Server> server) {
        if (!server)
        { return; }
        servers_[server->GetId()] = std::move(server);
    }

    std::shared_ptr<Server> DeviceRegistry::GetServer(const std::string& id) const {
        auto it = servers_.find(id);
        return (it == servers_.end()) ? nullptr : it->second;
    }

    std::vector<std::shared_ptr<Server>> DeviceRegistry::GetAllServers() const {
        std::vector<std::shared_ptr<Server>> out;
        out.reserve(servers_.size());
        for (auto& s : servers_)
        { out.push_back(s.second); }
        return out;
    }

    void DeviceRegistry::UpsertDevice(std::shared_ptr<IoTDevice> device) {
        if (!device)
        { return; }
        devices_[device->GetId()] = std::move(device);
    }

    std::shared_ptr<IoTDevice> DeviceRegistry::GetDevice(const std::string& id) const {
        auto it = devices_.find(id);
        return (it == devices_.end()) ? nullptr : it->second;
    }

    std::vector<std::shared_ptr<IoTDevice>> DeviceRegistry::GetAllDevices() const {
        std::vector<std::shared_ptr<IoTDevice>> out;
        out.reserve(servers_.size());
        for (auto& d : devices_)
        { out.push_back(d.second); }
        return out;
    }

    void DeviceRegistry::SetTopicsForDevice(const std::string& deviceId,
                                            std::vector<std::shared_ptr<MqttTopic>> topics) {
        topics_[deviceId] = std::move(topics);
    }

    std::vector<std::shared_ptr<MqttTopic>> DeviceRegistry::GetTopicsForDevice(
        const std::string& deviceId) const {
        auto it = topics_.find(deviceId);
        if (it == topics_.end())
            return {};
        return it->second;
    }

    std::vector<std::shared_ptr<MqttDevice>> DeviceRegistry::GetAllMqttDevices() const {
        std::vector<std::shared_ptr<MqttDevice>> out;
        for (auto t : devices_)
        {
            if (auto mqtt = std::dynamic_pointer_cast<MqttDevice>(t.second))
            { out.push_back(std::move(mqtt)); }
        }
        return out;
    }

}  // namespace FalcataIoTServer