/**
 * @file DeviceBuilder.hpp
 * @brief This is the declaration of the FalcataIoTServer::IoTDeviceBuilder
 * class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#pragma once

#include <Models/Core/IoTDevice.hpp>
#include <Factory/GenericFactory.hpp>

#include <memory>

namespace FalcataIoTServer
{
    class IoTDeviceBuilder
    {
    public:
        using Base = IoTDevice;

    public:
        static std::string Discriminator(const Postgresql::PgResult& res, int row);

        static std::unique_ptr<Base> Build(const std::string& protocol,
                                           const Postgresql::PgResult& res, int row);
    };

    using IoTDeviceFactory = GenericFactory<IoTDeviceBuilder>;
}  // namespace FalcataIoTServer