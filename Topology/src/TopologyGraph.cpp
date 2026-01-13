/**
 * @file TopologyGraph.cpp
 * @brief This module contains the implementation of the TopologyGraph class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */

#include <Topology/TopologyGraph.hpp>

namespace FalcataIoTServer
{
    struct TopologyGraph::Impl
    {
        /* data */
        std::unordered_map<std::string, std::shared_ptr<Site>> sites;
        std::unordered_map<std::string, std::shared_ptr<Zone>> zones;
        std::unordered_map<std::string, std::shared_ptr<Server>> servers;
        std::unordered_map<std::string, std::shared_ptr<IoTDevice>> devices;
        std::unordered_map<std::string, std::vector<std::shared_ptr<MqttTopic>>> deviceTopics;

        std::vector<SiteNode> siteList;

        Impl() = default;
        ~Impl() noexcept = default;
    };

    TopologyGraph::TopologyGraph() : CoreObject(), impl_(std::make_unique<Impl>()) {}

    TopologyGraph::~TopologyGraph() noexcept = default;

    void TopologyGraph::Clear() {
        // Clear all data
        impl_->sites.clear();
        impl_->zones.clear();
        impl_->servers.clear();
        impl_->devices.clear();
        impl_->deviceTopics.clear();
        impl_->siteList.clear();
    }

    const std::vector<SiteNode>& TopologyGraph::GetSites() const { return impl_->siteList; }

    void TopologyGraph::UpsertSite(const std::shared_ptr<Site>& site) {
        impl_->sites[site->Uuid_s()] = std::move(site);
    }

    void TopologyGraph::UpsertZone(const std::shared_ptr<Zone>& zone) {
        impl_->zones[zone->Uuid_s()] = std::move(zone);
    }

    void TopologyGraph::UpsertServer(const std::shared_ptr<Server>& server) {
        impl_->servers[server->Uuid_s()] = std::move(server);
    }

    void TopologyGraph::UpsertDevice(const std::shared_ptr<IoTDevice>& device) {
        impl_->devices[device->Uuid_s()] = std::move(device);
    }

    void TopologyGraph::SetTopics(const std::string& deviceId,
                                  const std::vector<std::shared_ptr<MqttTopic>>& topics) {
        impl_->deviceTopics[deviceId] = topics;
    }

    void TopologyGraph::Matrealize() {
        impl_->siteList.clear();
        for (auto& [sid, site] : impl_->sites)
        {
            SiteNode sn;
            sn.site = site;

            // Servers by metadata.site_id
            for (auto& [_, srv] : impl_->servers)
            {
                if (srv->GetMetadata()["site_id"] == sid)
                { sn.servers.push_back(srv); }
            }

            for (auto& [_, zone] : impl_->zones)
            {
                if (zone->GetSiteId() != sid)
                    continue;
                ZoneNode zn;
                zn.zone = zone;

                for (auto& [_, dev] : impl_->devices) {
                    if (dev->GetZone() != zone->Uuid_s()) continue;

                    DeviceNode dn;
                    dn.device = dev;
                    dn.topics = impl_->deviceTopics[dev->GetId()];
                    zn.devices.push_back(std::move(dn));
                }
                sn.zones.push_back(std::move(zn));
            }
            impl_->siteList.push_back(sn);
        }
    }

}  // namespace FalcataIoTServer