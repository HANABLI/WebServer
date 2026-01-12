// MqttBroker.hpp
#pragma once

#include <Models/Core/Server.hpp>
#include <MqttV5/MqttClient.hpp>
#include <StringUtils/StringUtils.hpp>
#include <MqttV5/MqttV5Packets.hpp>

namespace FalcataIoTServer
{
    class MqttBroker final : public Server
    {
        // Life cycle managment
    public:
        ~MqttBroker() noexcept = default;
        MqttBroker(const MqttBroker&) = delete;
        MqttBroker(MqttBroker&&) noexcept = default;
        MqttBroker& operator=(const MqttBroker&) = delete;
        MqttBroker& operator=(MqttBroker&&) noexcept = default;
        // Methods
    public:
        MqttBroker();
        MqttBroker(std::string id, std::string name, std::string host, uint16_t port,
                   std::string protocol, bool enabled, bool useTLS, std::string userName,
                   std::string password, bool cleanSession, bool willRetain, std::string willTopic,
                   std::string willPayload, MqttV5::QoSDelivery qos, uint16_t keepAlive,
                   MqttV5::Properties* props);
        MqttBroker(std::string name, std::string host, uint16_t port, std::string protocol,
                   bool enabled, bool useTLS, std::string userName, std::string password,
                   bool cleanSession, bool willRetain, std::string willTopic,
                   std::string willPayload, MqttV5::QoSDelivery qos, uint16_t keepAlive,
                   MqttV5::Properties* props);
        MqttBroker(std::string name, std::string host, uint16_t port, std::string protocol,
                   bool enabled);

        void AttachClient(std::shared_ptr<MqttV5::MqttClient>& client);

        bool IsReachable();
        void SetReachable(bool state);
        void SetDiagnosticsMessageDelegate(SystemUtils::DiagnosticsSender::Levels,
                                           std::string& msg);
        // Server
        std::shared_ptr<MqttV5::MqttClient::Transaction> Start() override;
        std::shared_ptr<MqttV5::MqttClient::Transaction> Stop() override;
        const std::string& GetServerType() const override;

        // Sérialisation générique d’un MqttBroker.
        Json::Value ToJson() const override;

        void FromJson(const Json::Value& json) override;

    private:
        struct Impl;
        std::unique_ptr<Impl> broker_;
    };
}  // namespace FalcataIoTServer