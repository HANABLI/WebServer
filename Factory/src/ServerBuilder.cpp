/**
 * @file ServerFactory.cpp
 * @brief This is the implementation of the FalcataIoTServer::ServerBuilder
 * class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#include <MqttV5/MqttV5Types.hpp>
#include <Factory/ServerBuilder.hpp>

namespace FalcataIoTServer
{
    std::string ServerBuilder::Discriminator(const Postgresql::PgResult& res, int row) {
        return res.TextRequired(row, "protocol");
    }
    // Bulk mapping: build one server
    std::unique_ptr<ServerBuilder::Base> ServerBuilder::Build(const std::string& protocol,
                                                              const Postgresql::PgResult& res,
                                                              int row) {
        if (row < 0 || row >= res.Rows())
        { throw std::runtime_error("ServerFactory::FromRawRow: row out of range"); }

        std::string id = res.TextRequired(row, "id");
        std::string name = res.TextRequired(row, "name");
        std::string proto = res.TextRequired(row, "protocol");
        bool enabled = res.Bool(row, "enabled", false);
        std::string host = res.Text(row, "host", "localhost");

        if (protocol == "mqtt")
        {
            int port = res.Int(row, "port", 1883);
            bool useTLS = res.Bool(row, "useTLS", false);
            std::string userName = res.TextRequired(row, "userName");
            std::string password = res.TextRequired(row, "password");
            bool cleanSession = res.Bool(row, "cleanSession", true);
            bool willRetain = res.Bool(row, "willRetain", false);
            std::string willTopic = res.TextRequired(row, "willTopic");
            std::string willPayload = res.TextRequired(row, "willPayload");
            int qos = res.Int(row, "qos", 1);
            int keepAlive = res.Int(row, "keepAlive", 30);
            return std::make_unique<MqttBroker>(
                id, name, host, (uint16_t)port, proto, enabled, useTLS, userName, password,
                cleanSession, willRetain, willTopic, willPayload, (MqttV5::QoSDelivery)qos,
                (uint16_t)keepAlive,
                nullptr);  // TODO (HNA) : create properties table and query out the corresponding
                           // properties row
        }

        else if (protocol == "modbus-tcp")
        {
            int port = res.Int(row, "port", 502);
            return nullptr;  // std::make_unique<ModbusServer>(id, name, host, port, enabled);
        } else if (protocol == "opcua")
        {
            return nullptr;  // std::make_unique<OpcUaServer>(id, name, endpoint, enabled); }
        }
        throw std::runtime_error("ServerFactory::FromRawRow: unknown protocol: " + protocol);
    }
}  // namespace FalcataIoTServer
