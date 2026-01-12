/**
 * @file ServerBuilder.hpp
 * @brief This is the declaration of the FalcataIoIServer::ServerBuilder
 * class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#pragma once

#include <Models/Core/Server.hpp>
#include <Factory/GenericFactory.hpp>
#include <Models/Servers/Modbus.hpp>
#include <Models/Servers/MqttBroker.hpp>
#include <Models/Servers/OpcUaServer.hpp>

#include <memory>

namespace FalcataIoTServer
{
    class ServerBuilder
    {
    public:
        using Base = Server;

    public:
        static std::string Discriminator(const Postgresql::PgResult& res, int row);

        static std::unique_ptr<Base> Build(const std::string& protocol,
                                           const Postgresql::PgResult& res, int row);
    };

    using ServerFactory = GenericFactory<ServerBuilder>;
}  // namespace FalcataIoTServer