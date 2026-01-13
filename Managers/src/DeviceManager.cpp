/**
 * @file DeviceManager.cpp
 * @brief This represent the implementation of the FalcataIoTServer::DeviceManager class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#include <Managers/DeviceManager.hpp>
#include <memory>
namespace FalcataIoTServer
{
    struct DeviceManager::Impl
    {
        std::shared_ptr<Postgresql::PgClient> pg;
        std::shared_ptr<MqttV5::MqttClient> client;

        ServerRepository serverRepo;
        IoTDeviceRepository deviceRepo;
        MqttTopicRepository topicRepo;

        DeviceRegistry registry;

        std::unordered_map<std::string, std::unique_ptr<MqttDeviceConnector>> mqttConnectors;

        Impl(std::shared_ptr<Postgresql::PgClient> pg, std::shared_ptr<MqttV5::MqttClient> client) :
            pg(pg), client(client), serverRepo(pg), deviceRepo(pg), topicRepo(pg) {}
        ~Impl() noexcept = default;

        void LoadServers() {
            auto rows = serverRepo.FindAll();
            for (auto& up : rows)
            {
                if (!up)
                    continue;
                std::shared_ptr<Server> sp(up.release());
                registry.UpsertServer(std::move(sp));
            }
        }

        void LoadDevices() {
            auto rows = deviceRepo.FindAll();
            for (auto& up : rows)
            {
                if (!up)
                    continue;
                std::shared_ptr<IoTDevice> sp(up.release());
                registry.UpsertDevice(std::move(sp));
            }
        }

        void LoadTopics() {
            auto& rows = topicRepo.FindAll();
            std::unordered_map<std::string, std::vector<std::shared_ptr<MqttTopic>>> byDevice;

            for (auto& up : rows)
            {
                if (!up)
                    continue;
                std::shared_ptr<MqttTopic> tp(up.release());
                const std::string deviceId = tp->GetDeviceId();
                byDevice[deviceId].push_back(std::move(tp));
            }

            for (auto& kv : byDevice)
            {
                const std::string& devId = kv.first;
                auto topics = kv.second;

                registry.SetTopicsForDevice(devId, topics);

                auto dev = registry.GetDevice(devId);
                if (!dev)
                    continue;

                if (auto mqttDev = std::dynamic_pointer_cast<MqttDevice>(dev))
                {
                    std::set<std::shared_ptr<MqttTopic>> setTopics;
                    for (auto& t : topics) setTopics.insert(t);
                    mqttDev->SetTopics(setTopics);
                }
            }
        }

        void BuildMqttConnectors() {
            for (auto& s : registry.GetAllServers())
            {
                if (!s || s->GetProtocol() != "mqtt")
                    continue;

                auto broker = std::dynamic_pointer_cast<MqttBroker>(s);
                if (!broker)
                    continue;

                broker->AttachClient(client);
                auto t = broker->Start();
                if (t->AwaitCompletion(std::chrono::milliseconds(100)))
                {  // TODO set broker connection timeout
                    switch (t->transactionState)
                    {
                    case MqttV5::IMqttV5Client::Transaction::State::Success: {
                        std::string msg = "Connection established.";
                        broker->SetDiagnosticsMessageDelegate(
                            SystemUtils::DiagnosticsSender::Levels::INFO, msg);
                        // TODO: update DB with server state
                        // serverRepo.Update(broker);
                    }
                    break;
                    case MqttV5::IMqttV5Client::Transaction::State::ShunkedPacket: {
                        std::string msg = "ShunkedPacket.";
                        broker->SetDiagnosticsMessageDelegate(
                            SystemUtils::DiagnosticsSender::Levels::WARNING, msg);
                        // TODO: update DB with server state
                        // serverRepo.Update(broker);
                    }
                    break;
                    default:
                        break;
                    }
                }

                mqttConnectors[broker->GetId()] =
                    std::make_unique<MqttDeviceConnector>(client, broker);
                // TODO update database
            }
        }
    };

    DeviceManager::DeviceManager(std::shared_ptr<Postgresql::PgClient> pg,
                                 std::shared_ptr<MqttV5::MqttClient> client) :
        impl_(std::make_unique<Impl>(pg, client)) {}

    void DeviceManager::ReloadAll() {
        impl_->registry.Clear();
        impl_->mqttConnectors.clear();

        impl_->LoadServers();
        impl_->LoadDevices();
        impl_->LoadTopics();

        impl_->BuildMqttConnectors();
    }

    DeviceRegistry& DeviceManager::Registry() { return impl_->registry; }

    const DeviceRegistry& DeviceManager::Registry() const { return impl_->registry; }
    //
    void DeviceManager::SyncAllMqttDevices() {
        for (auto& dev : impl_->registry.GetAllMqttDevices())
        {
            if (!dev || !dev->IsEnabled())
                continue;

            const std::string brokerId = dev->GetServerId();
            auto it = impl_->mqttConnectors.find(brokerId);
            if (it == impl_->mqttConnectors.end())
                continue;
            it->second->SyncDevice(*dev);
        }
    }

    std::shared_ptr<MqttV5::MqttClient::Transaction> DeviceManager::PublishToBroker(
        const std::string& serverId, const std::string& topic, const std::string& payload,
        const bool retain, MqttV5::QoSDelivery qos, const uint16_t packetID,
        MqttV5::Properties* properties) {
        auto broker = std::dynamic_pointer_cast<MqttBroker>(impl_->registry.GetServer(serverId));
        if (!broker)
            return false;
        if (!impl_->client || broker->IsReachable())
        { return false; }

        auto t = impl_->client->Publish(serverId, topic, payload, retain, qos, packetID, nullptr);

        return t;
    }
}  // namespace FalcataIoTServer