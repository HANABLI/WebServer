/**
 * @file TopologyUpdate.cpp
 * @brief This is the implementation of the FalcataIoTServer::TopologyUpdate class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#include <SystemUtils/Time.hpp>
#include <Topology/TopologyUpdate.hpp>
#include <WebSocket/WebSocket.hpp>
#include <Models/Location/Site.hpp>
#include <Models/Location/Zone.hpp>
#include <TimeTracking/TimeTracking.hpp>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>

namespace FalcataIoTServer
{
    struct TopologyUpdater::Impl
    {
        std::shared_ptr<Postgresql::PgClient> pg;
        std::shared_ptr<DeviceManager> devicesMgr;
        std::shared_ptr<WebSocket::WebSocket> ws;

        std::atomic<bool> ranning = false;
        std::thread worker;
        std::shared_ptr<TopologyGraph> graph;

        Impl(std::shared_ptr<Postgresql::PgClient>& pg, std::shared_ptr<DeviceManager>& dm,
             std::shared_ptr<WebSocket::WebSocket>& ws) :
            pg(std::move(pg)), devicesMgr(std::move(dm)), ws(std::move(ws)) {}
        ~Impl() noexcept = default;

        void Reload() {
            devicesMgr->ReloadAll();
            devicesMgr->SyncAllMqttDevices();

            graph->Clear();
            for (auto& s : devicesMgr->Registry().GetAllSites())
            { graph->UpsertSite(s); }
            for (auto& z : devicesMgr->Registry().GetAllZones())
            { graph->UpsertZone(z); }
            for (auto& sv : devicesMgr->Registry().GetAllServers())
            { graph->UpsertServer(sv); }
            for (auto& dev : devicesMgr->Registry().GetAllDevices())
            {
                graph->UpsertDevice(dev);
                graph->SetTopics(dev->GetId(),
                                 devicesMgr->Registry().GetTopicsForDevice(
                                     dev->GetId()));  // TODO select by protocol
            }
            graph->Matrealize();

            if (ws)
            {
                Json::Value msg;
                msg.Set("type", "topologie.update");
                Json::Value sites;
                for (auto& s : graph->GetSites())
                {
                    Json::Value siteObj = s.site->ToJson();
                    Json::Value zones;

                    for (auto& z : s.zones)
                    {
                        Json::Value zoneObj = z.zone->ToJson();
                        Json::Value devices;

                        for (auto& d : z.devices)
                        {
                            Json::Value devObj = d.device->ToJson();
                            Json::Value topics;

                            for (auto& t : d.topics)
                            { topics.Add(t->ToJson()); }

                            devObj.Set("topics", topics);
                            devices.Set(d.device->GetId(), devObj);
                        }

                        zoneObj.Set("devices", devices);
                        zones.Set(z.zone->Uuid_s(), zoneObj);
                    }

                    siteObj.Set("zones", zones);
                    sites.Set(s.site->Uuid_s(), siteObj);
                }
                msg.Set("sites", sites);
                std::shared_ptr<SystemUtils::Time> time;
                msg.Set("ts", static_cast<float>(time->GetTime()));
                ws->SendText(msg);  // TODO send brodcast msg
            }
        }
    };

    TopologyUpdater::TopologyUpdater(std::shared_ptr<Postgresql::PgClient>& pg,
                                     std::shared_ptr<DeviceManager>& dm,
                                     std::shared_ptr<WebSocket::WebSocket>& ws) :
        impl_(std::make_unique<Impl>(pg, dm, ws)) {}

    TopologyUpdater::~TopologyUpdater() noexcept = default;

    const TopologyGraph& TopologyUpdater::CurrentTopology() const { return *impl_->graph; }
    void TopologyUpdater::Start() {
        if (impl_->ranning)
            return;
        impl_->ranning = true;
        impl_->Reload();
        impl_->worker = std::thread(
            [this]
            {
                while (impl_->ranning)
                {
                    impl_->pg->Listen("iot_changes", [this]() { impl_->Reload(); });
                }
            });
    }

    void TopologyUpdater::Stop() {
        impl_->ranning = false;
        if (impl_->worker.joinable())
        { impl_->worker.join(); }
    }
}  // namespace FalcataIoTServer