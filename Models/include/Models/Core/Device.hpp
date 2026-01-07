/**
 * @file Device.hpp
 * @brief This is a CRTP trait that provide a full implementation
 * of the IDevice interface.
 * @copyright © copyright 2025 by Hatem Nabli
 */
#pragma once

#include <Models/Core/IDevice.hpp>
#include <Json/Json.hpp>

#include <memory>

namespace FalcataIoTServer
{
    template <typename Derived>
    class Device : public IDevice, public IJsonSerializable
    {
        // Methods
    public:
        ~Device() override = default;
        Device() = default;

        const std::string Device::GetId() const override { return self().Uuid_s(); }
        void Device::SetId(const std::string& id) override { self().UuidFromString(id); }

        const std::string Device::GetName() const override { return self().impl_->name; }
        void Device::SetName(const std::string& name) override { self().impl_->name = name; }

        // "kind" = catégorie fonctionnelle (server, sensor, gateway, etc.)
        const std::string Device::GetKind() const override { return self().impl_->kind; }

        // "protocol" = mqtt, opcua, modbus-tcp…
        const std::string Device::GetProtocol() const override { return self().impl_->protocol; }

        // Actif / inactif
        bool Device::IsEnabled() const override { return self().impl_->enabled; }
        void Device::SetEnabled(bool enabled) override { self().impl_->enabled = enabled; }

        // Sérialisation JSON pour ton API REST
        Json::Value Device::ToJson() const override {
            const auto& impl = self().impl_;
            Json::Value device(Json::Value::Type::Object);
            device.Set("id", self().Uuid_s());
            device.Set("name", impl->name);
            device.Set("kind", impl->kind);
            device.Set("protocol", impl->protocol);
            device.Set("enabled", impl->enabled);

            return device;
        }

        void Device::FromJson(const Json::Value& json) override {
            auto& impl = self().impl_;

            if (json.Has("id"))
            { self().UuidFromString(json["id"].ToEncoding()); }
            if (json.Has("name"))
            { impl->name = json["name"]; }
            if (json.Has("kind"))
            { impl->kind = json["kind"]; }
            if (json.Has("protocol"))
            { impl->protocol = json["protocol"]; }
            if (json.Has("enabled"))
            { impl->enabled = json["enabled"]; }
        }

    protected:
        Derived& self() { return static_cast<Derived&>(*this); }
        const Derived& self() const { return static_cast<const Derived&>(*this); }
    };
}  // namespace FalcataIoTServer
