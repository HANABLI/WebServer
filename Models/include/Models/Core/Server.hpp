// Server.hpp
#pragma once
/**
 * @file Server.hpp
 * @brief This is the declaration of the Server base class that
 * heritate both CoreObject and CRTP classes
 */
#include <Models/Core/Device.hpp>
#include <Models/Core/IoTDevice.hpp>
#include <Models/Core/CoreObject.hpp>
#include <Models/Core/Protocol.hpp>
#include <MqttV5/MqttClient.hpp>

#include <cstdint>
#include <vector>

namespace FalcataIoTServer
{
    class Server : public CoreObject, public Device<Server>
    {
    public:
        // lifecycle managements
        virtual ~Server() noexcept = default;
        Server(const Server&) = delete;
        Server(Server&&) noexcept = default;
        Server& operator=(const Server&) = delete;
        Server& operator=(Server&&) noexcept = default;

    public:
        // Methods
        explicit Server();
        Server(const Protocol&);
        Server(std::string id, std::string name, std::string host, uint16_t port,
               std::string protocol, bool enabled);
        Server(std::string name, std::string host, uint16_t port, std::string protocol,
               bool enabled);

        // Host / Port communs à tous les serveurs
        const std::string GetHost() const;
        void SetHost(const std::string& host);

        uint16_t GetPort() const;
        void SetPort(uint16_t port);
        const std::vector<std::string>& GetTags() const;
        void SetTags(std::vector<std::string> v);
        void AddTag(const std::string& tag);
        bool GetUseTls() const;
        void SetUseTls(bool v);
        const Json::Value& GetMetadata() const;
        void SetMetadata(const Json::Value& v);

        const std::string& GetCreatedAt() const;
        void SetCreatedAt(const std::string& v);

        const std::string& GetUpdatedAt() const;
        void SetUpdatedAt(const std::string& v);

        // std::set<std::shared_ptr<IoTDevice>> GetIotDevices() const;
        // void SetIotDevices(std::set<std::shared_ptr<IoTDevice>>& devices);
        // void AddIotDevice(std::shared_ptr<IoTDevice>& device);
        // void RemoveIotDevice(std::shared_ptr<IoTDevice>& device);

        const std::vector<std::string> GetInsertParams() const override;
        const std::vector<std::string> GetUpdateParams() const override;
        const std::vector<std::string> GetRemoveParams() const override;
        const std::vector<std::string> GetDisableParams() const override;

        // Sérialisation générique d’un "Server"
        Json::Value ToJson() const override;

        void FromJson(const Json::Value& json) override;

        // Méthode pure virtuelle pour forcer les dérivés à s’identifier
        // (par ex: "mqtt-broker", "opcua-server"…)
        virtual const std::string& GetServerType() const = 0;
        // API spécifique aux serveurs
        virtual std::shared_ptr<MqttV5::MqttClient::Transaction> Start() = 0;
        virtual std::shared_ptr<MqttV5::MqttClient::Transaction> Stop() = 0;

    public:
        // Protégé pour empêcher l’instanciation directe
        struct Impl
        {
            std::string name;
            std::string kind;      // "server"
            std::string protocol;  // "mqtt", "opcua", "modbus-tcp"…
            bool enabled = true;

            std::string host = "localhost";
            uint16_t port = 0;
            bool useTls = false;
            std::vector<std::string> tags;
            Json::Value metadata = Json::Value(Json::Value::Type::Object);

            std::string createdAt;
            std::string updatedAt;
        };

        std::unique_ptr<Impl> impl_;
    };

}  // namespace FalcataIoTServer