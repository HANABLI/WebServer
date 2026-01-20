/**
 * @file CommandDispatcher.cpp
 * @brief This module contains the implementation of the FalcataIoTServer::CommandDispatcher class.
 * @copyright © 2025 by Hatem Nabli.
 */

#include <Commands/CommandDispatcher.hpp>

namespace FalcataIoTServer
{
    struct CommandDispatcher::Impl
    {
        std::shared_ptr<CommandRepo> repo;
        std::shared_ptr<DeviceManager> devicesMgr;

        std::atomic<bool> running = false;
        std::thread worker;

        std::function<void(const std::string& jsonText)> wsBroadcaster;

        Impl(std::shared_ptr<CommandRepo>& commandRepo, std::shared_ptr<DeviceManager>& dm) :
            repo(std::move(commandRepo)), devicesMgr(std::move(dm)) {}
        ~Impl() noexcept = default;

        void DispatchPending(int limit) {
            auto pendingCommands = repo->FetchPending(limit);
            for (auto& cmdPtr : pendingCommands)
            {
                auto& cmd = *cmdPtr;
                auto dev = devicesMgr->Registry().GetDevice(cmd.GetDeviceId());
                if (!dev)
                {
                    repo->MarkFailed(cmd.Uuid_s(), "device_not_found");
                    continue;
                }

                auto mqttdev = std::dynamic_pointer_cast<MqttDevice>(dev);
                if (!mqttdev)
                {
                    repo->MarkFailed(cmd.Uuid_s(), "not_mqtt_device");
                    continue;
                }
                std::string topic;
                MqttV5::QoSDelivery qos = MqttV5::QoSDelivery::AtLeastOne;
                bool retain = false;

                for (auto& c : mqttdev->GetTopics())
                {
                    if (c->GetRole() == "command" && c->GetTopic() == cmd.GetCommand() &&
                        c->GetDirection() == "pub")
                    {
                        topic = c->GetTopic();
                        qos = c->GetQoS();
                        retain = c->GetRetain();
                        break;
                    }
                }

                if (topic.empty())
                {
                    repo->MarkFailed(cmd.Uuid_s(), "no_command_topic");
                    continue;
                }

                Json::Value payload;
                payload.Set("cmd_id", cmd.Uuid_s());
                payload.Set("command", cmd.GetCommand());
                payload.Set("params", cmd.GetParams());

                auto t =
                    devicesMgr->PublishToBroker(dev->GetServerId(), topic, payload.ToEncoding(),
                                                retain, qos, cmd.Uuid_r().ToUint16(), nullptr);
                if (t)
                {
                    repo->MarkSent(cmd.Uuid_s());
                    if (wsBroadcaster)
                    {
                        auto sentCommand = repo->GetById(cmd.Uuid_s());
                        if (sentCommand)
                        {
                            auto j = Json::Value(Json::Value::Type::Object);
                            j.Set("type", "command.sent");
                            j.Set("command", sentCommand->ToJson());
                            wsBroadcaster(j.ToEncoding());
                        }
                    }
                }
                if (t->AwaitCompletion(std::chrono::milliseconds(200)))
                {  // TODO set broker connection timeout
                    switch (t->transactionState)
                    {
                    case MqttV5::IMqttV5Client::Transaction::State::Success: {
                        // std::string msg = StringUtils::sprintf(
                        //     "topic : %s published successfully to broker %s .", topic, serverId);
                        // broker->SetDiagnosticsMessageDelegate(
                        //     SystemUtils::DiagnosticsSender::Levels::INFO, msg);
                        repo->MarkAcked(cmd.Uuid_s());
                        if (wsBroadcaster)
                        {
                            auto ackCommand = repo->GetById(cmd.Uuid_s());
                            if (ackCommand)
                            {
                                auto j = Json::Value(Json::Value::Type::Object);
                                j.Set("type", "command.ack");
                                j.Set("command", ackCommand->ToJson());
                                wsBroadcaster(j.ToEncoding());
                            }
                        }
                    }
                    break;
                    case MqttV5::IMqttV5Client::Transaction::State::ShunkedPacket: {
                        // std::string msg = StringUtils::sprintf(
                        //     "topic : %s published to broker %s with shunkedPacket.", topic,
                        //     serverId);
                        // broker->SetDiagnosticsMessageDelegate(
                        //     SystemUtils::DiagnosticsSender::Levels::WARNING, msg);
                        repo->MarkFailed(cmd.Uuid_s(), "publish_failed_shunkedPacket");
                        if (wsBroadcaster)
                        {
                            auto failedCommand = repo->GetById(cmd.Uuid_s());
                            if (failedCommand)
                            {
                                auto j = Json::Value(Json::Value::Type::Object);
                                j.Set("type", "command.failed");
                                j.Set("command", failedCommand->ToJson());
                                wsBroadcaster(j.ToEncoding());
                            }
                        }
                    }
                    break;
                    default:
                        break;
                    }
                }
            }
        }
    };

    CommandDispatcher::CommandDispatcher(std::shared_ptr<CommandRepo>& repo,
                                         std::shared_ptr<DeviceManager>& dm) :
        impl_(std::make_unique<Impl>(repo, dm)) {}

    CommandDispatcher::~CommandDispatcher() noexcept = default;

    void CommandDispatcher::Start() {
        if (impl_->running)
            return;
        impl_->running = true;
        impl_->DispatchPending(200);
        impl_->worker = std::thread(
            [this]
            {
                while (impl_->running)
                {
                    impl_->repo->Listen("iot_commands", [this]() { impl_->DispatchPending(200); });
                    std::this_thread::sleep_for(std::chrono::microseconds(500));
                }
            });
    }

    void CommandDispatcher::Stop() {
        if (!impl_->running)
            return;
        impl_->running = false;
        if (impl_->worker.joinable())
        { impl_->worker.join(); }
        // Implementation to stop the dispatcher
    }

    void CommandDispatcher::OnDbSignal() { impl_->DispatchPending(200); }

    void CommandDispatcher::SetWsBroadcaster(
        std::function<void(const std::string& jsonText)> wsFn) {
        impl_->wsBroadcaster = std::move(wsFn);
    }
}  // namespace FalcataIoTServer