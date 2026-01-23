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

        const std::string GetId() const override { return self().Uuid_s(); }
        void SetId(const std::string& id) override { self().UuidFromString(id); }

        const std::string GetName() const override { return self().impl_->name; }
        void SetName(const std::string& name) override { self().impl_->name = name; }

        // "kind" = catégorie fonctionnelle (server, sensor, gateway, etc.)
        const std::string GetKind() const override { return self().impl_->kind; }
        void SetKind(const std::string& kind) override { self().impl_->kind = kind; }

        // "protocol" = mqtt, opcua, modbus-tcp…
        const std::string GetProtocol() const override { return self().impl_->protocol; }
        void SetProtocol(const std::string& protocol) override {
            self().impl_->protocol = protocol;
        }
        // Actif / inactif
        bool IsEnabled() const override { return self().impl_->enabled; }
        void SetEnabled(bool enabled) override { self().impl_->enabled = enabled; }

        // Sérialisation JSON pour ton API REST
        virtual Json::Value ToJson() const override {
            const auto& impl = self().impl_;
            Json::Value device(Json::Value::Type::Object);
            device.Set("id", self().Uuid_s());
            device.Set("name", impl->name);
            device.Set("kind", impl->kind);
            device.Set("protocol", impl->protocol);
            device.Set("enabled", impl->enabled);

            return device;
        }

        virtual void FromJson(const Json::Value& json) override {
            auto& impl = self().impl_;

            if (json.Has("id"))
            { SetId(json["id"]); }
            if (json.Has("name"))
            { SetName(json["name"]); }
            if (json.Has("kind"))
            { SetKind(json["kind"]); }
            if (json.Has("protocol"))
            { SetProtocol(json["protocol"]); }
            if (json.Has("enabled"))
            { SetEnabled(json["enabled"]); }
        }

    protected:
        Derived& self() { return static_cast<Derived&>(*this); }
        const Derived& self() const { return static_cast<const Derived&>(*this); }
    };
}  // namespace FalcataIoTServer
