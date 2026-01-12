/**
 * @file Event.hpp
 */
#pragma once

#include <Models/Core/CoreObject.hpp>
#include <Models/Core/IJsonSerialisable.hpp>
#include <memory>
#include <string>

namespace FalcataIoTServer
{
    class Event final : public CoreObject, public IJsonSerializable
    {
        // Life cycle managment
    public:
        ~Event();
        Event(const Event&) = delete;
        Event(Event&&) noexcept = default;
        Event& operator=(const Event&) = delete;
        Event& operator=(Event&&) noexcept = default;

        // Methods
    public:
        Event(/* args */);

        const std::string& CorrelationId() const;
        void CorrelationId(const std::string& v);
        const std::string& Ts() const;
        void Ts(const std::string& v);
        const std::string& Source() const;
        void Source(const std::string& v);
        const std::string& Type() const;
        void Type(const std::string& v);
        const std::string& Severity() const;
        void Severity(const std::string& v);
        const std::string& SiteId() const;
        void SiteId(const std::string& v);
        const std::string& ZoneId() const;
        void ZoneId(const std::string& v);
        const std::string& DeviceId() const;
        void DeviceId(const std::string& v);
        const std::string& GetCreatedAt() const;
        void SetCreatedAt(const std::string& v);

        const Json::Value& Payload() const;
        void Payload(const Json::Value& v);

        const std::vector<std::string> GetInsertParams() const override;
        const std::vector<std::string> GetUpdateParams() const override;
        const std::vector<std::string> GetRemoveParams() const override;
        const std::vector<std::string> GetDisableParams() const override;

        // IJsonSerializable
        Json::Value ToJson() const override;
        void FromJson(const Json::Value& json) override;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

}  // namespace FalcataIoTServer