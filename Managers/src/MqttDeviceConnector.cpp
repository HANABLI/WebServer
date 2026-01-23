/**
 * @file MqttDeviceConnector.cpp
 * @brief This represent the implementation of the FalcataIoTServer::MqttDeviceConnector class.
 * @copyright copyright © 2025 by Hatem Nabli.
 */

#include <Managers/MqttDeviceConnector.hpp>
#include <memory>
#include <sstream>

namespace FalcataIoTServer
{
    struct MqttDeviceConnector::Impl
    {
        std::shared_ptr<MqttV5::MqttClient> client;
        std::shared_ptr<MqttBroker> broker;

        std::unordered_set<std::string> subscribedTopics;

        Impl(std::shared_ptr<MqttV5::MqttClient> client, std::shared_ptr<MqttBroker> broker) :
            client(client), broker(broker) {}
        ~Impl() noexcept = default;

        bool ShouldSubscribe(const std::shared_ptr<MqttTopic> tp) {
            if (!tp->IsEnabled())
                return false;
            const auto& dir = tp->GetDirection();  // pub | sub |pubsub
            return (dir == "sub" || dir == "pubsub");
        }
    };

    MqttDeviceConnector::MqttDeviceConnector(std::shared_ptr<MqttV5::MqttClient> client,
                                             std::shared_ptr<MqttBroker> broker) :
        impl_(std::make_unique<Impl>(client, broker)) {}

    MqttDeviceConnector::~MqttDeviceConnector() noexcept = default;

    void MqttDeviceConnector::SyncDevice(const MqttDevice& dev) {
        if (!impl_->client || !impl_->broker->IsReachable())
            return;  // TODO diagnosticSibscriber
        if (!dev.IsEnabled())
            return;

        for (const auto& tp : dev.GetTopics())
        {
            if (!tp)
                continue;
            if (!impl_->ShouldSubscribe(tp))
                continue;

            const std::string& topic = tp->GetTopic();
            if (topic.empty())
                continue;

            if (impl_->subscribedTopics.insert(tp->GetId()).second)
            {
                auto transaction = impl_->client->Subscribe(
                    impl_->broker->GetId(), tp->GetTopic().c_str(), tp->GetRetain(),
                    tp->WithAutoFeedBack(), tp->GetQoS(), tp->GetRetainAsPublished(),
                    nullptr);  // TODO properties from broker
                if (transaction->AwaitCompletion(std::chrono::milliseconds(30)))
                {  // TODO set broker connection timeout
                    switch (transaction->transactionState)
                    {
                    case MqttV5::IMqttV5Client::Transaction::State::Success:
                        impl_->broker->SetDiagnosticsMessageDelegate(
                            SystemUtils::DiagnosticsSender::Levels::INFO,
                            StringUtils::sprintf("Subscribed topic : %s.", tp->GetTopic().c_str()));
                        // serverRepo.Update(broker);

                        // TODO create event
                        break;
                    case MqttV5::IMqttV5Client::Transaction::State::ShunkedPacket:
                        impl_->broker->SetDiagnosticsMessageDelegate(
                            SystemUtils::DiagnosticsSender::Levels::WARNING,
                            StringUtils::sprintf("Topic subscription error : %s .",
                                                 tp->GetTopic().c_str()));
                        // serverRepo.Update(broker);
                        // TODO create event
                        break;
                    default:
                        break;
                    }
                }
            }
        }
    }

    void MqttDeviceConnector::UnsyncDevice(const MqttDevice& dev) {
        if (!impl_->client || !impl_->broker->IsReachable())
            return;

        for (const auto& tp : dev.GetTopics())
        {
            if (!tp)
                continue;
            const std::string& topic = tp->GetTopic();
            if (topic.empty())
                continue;

            if (impl_->subscribedTopics.erase(tp->GetId()) > 0)
            { impl_->client->Unsubscribe(tp->BuildUnsubTopic().get(), nullptr); }
        }
    }

}  // namespace FalcataIoTServer