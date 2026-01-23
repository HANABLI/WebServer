#ifndef MQTT_PLUGIN_DEVICE_HPP
#define MQTT_PLUGIN_DEVICE_HPP

// IDevice.hpp
#pragma once

#include <string>
#include <Json/Json.hpp>

namespace FalcataIoTServer
{
    class IDevice
    {
    public:
        virtual ~IDevice() = default;

        // Identité / métadonnées
        virtual const std::string GetId() const = 0;
        virtual void SetId(const std::string& id) = 0;

        virtual const std::string GetName() const = 0;
        virtual void SetName(const std::string& name) = 0;

        // "kind" = catégorie fonctionnelle (server, sensor, gateway, etc.)
        virtual const std::string GetKind() const = 0;
        virtual void SetKind(const std::string& kind) = 0;

        // "protocol" = mqtt, opcua, modbus-tcp…
        virtual const std::string GetProtocol() const = 0;
        virtual void SetProtocol(const std::string& protocol) = 0;
        // Actif / inactif
        virtual bool IsEnabled() const = 0;
        virtual void SetEnabled(bool enabled) = 0;
    };
}  // namespace FalcataIoTServer
#endif