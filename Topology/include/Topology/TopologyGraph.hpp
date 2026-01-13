#pragma once
/**
 * @file TopologyGraph.hpp
 * @brief This module contains the declaration of the FalcataIoTServer::TopologyGraph class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */

#include <Models/Core/IoTDevice.hpp>
#include <Models/Location/Zone.hpp>
#include <Models/Location/Site.hpp>
#include <Models/Core/Server.hpp>
#include <Models/Data/MqttTopic.hpp>

#include <memory>
#include <vector>
#include <unordered_map>

namespace FalcataIoTServer
{
    struct DeviceNode
    {
        std::shared_ptr<IoTDevice> device;
        std::vector<std::shared_ptr<MqttTopic>> topics;
    };

    struct ZoneNode
    {
        std::shared_ptr<Zone> zone;
        std::vector<DeviceNode> devices;
    };

    struct SiteNode
    {
        std::shared_ptr<Site> site;
        std::vector<std::shared_ptr<Server>> servers;
        std::vector<ZoneNode> zones;
    };

    class TopologyGraph : public CoreObject
    {
    public:
        // Life cycle management
        TopologyGraph(const TopologyGraph&) = delete;
        TopologyGraph(TopologyGraph&&) noexcept = default;
        TopologyGraph& operator=(const TopologyGraph&) = delete;
        TopologyGraph& operator=(TopologyGraph&&) noexcept = default;
        ~TopologyGraph() noexcept;

        // Methods
    public:
        explicit TopologyGraph();

        void Clear();

        void UpsertSite(const std::shared_ptr<Site>& site);
        void UpsertZone(const std::shared_ptr<Zone>& zone);
        void UpsertServer(const std::shared_ptr<Server>& server);
        void UpsertDevice(const std::shared_ptr<IoTDevice>& device);
        void SetTopics(const std::string& deviceId,
                       const std::vector<std::shared_ptr<MqttTopic>>& topics);

        void Matrealize();

        const std::vector<SiteNode>& GetSites() const;

    private:
        // data
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}  // namespace FalcataIoTServer