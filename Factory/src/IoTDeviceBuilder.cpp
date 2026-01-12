/**
 * @file ServerFactory.cpp
 * @brief This is the implementation of the FalcataIoTServer::IoTDeviceBuilder
 * class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#include <MqttV5/MqttV5Types.hpp>
#include <Factory/IoTDeviceBuilder.hpp>
#include <Models/IoTDevices/MqttDevice.hpp>
#include <memory>

namespace FalcataIoTServer
{
    std::string IoTDeviceBuilder::Discriminator(const Postgresql::PgResult& res, int row) {
        return res.TextRequired(row, "protocol");
    }
    // Bulk mapping: build one server
    std::unique_ptr<IoTDeviceBuilder::Base> IoTDeviceBuilder::Build(const std::string& protocol,
                                                                    const Postgresql::PgResult& res,
                                                                    int row) {
        if (row < 0 || row >= res.Rows())
        { throw std::runtime_error("IoTDeviceBuilder::FromRawRow: row out of range"); }
        if (protocol == "mqtt")
        {
            std::string id = res.TextRequired(row, "id");
            std::string serverId = res.TextRequired(row, "serverId");
            std::string name = res.TextRequired(row, "name");
            std::string kind = res.TextRequired(row, "kind");
            std::string proto = res.TextRequired(row, "protocol");
            bool enabled = res.Bool(row, "enabled", false);
            std::string zoneId = res.TextRequired(row, "zoneId");
            return std::make_unique<MqttDevice>(id, serverId, name, kind, proto, enabled, zoneId);
        }
        throw std::runtime_error("IoTDeviceBuilder::FromRawRow: unknown serverId: " + protocol);
    }
}  // namespace FalcataIoTServer
