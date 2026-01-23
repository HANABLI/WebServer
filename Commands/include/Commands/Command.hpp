#pragma once
/**
 * @file Command.hpp
 * @brief This is the declaration of the FalcataIoTServer::Command class.
 * @copyright  copyright © 2025 by Hatem Nabli.
 */

#include <Models/Core/CoreObject.hpp>
#include <Models/Data/MqttTopic.hpp>
#include <Json/Json.hpp>
#include <memory>

namespace FalcataIoTServer
{
    class Command : public CoreObject, public IJsonSerializable
    {
    public:
        ~Command() noexcept;
        Command(const Command&) = delete;
        Command(Command&&) noexcept = default;
        Command& operator=(const Command&) = delete;
        Command& operator=(Command&&) noexcept = default;

    public:
        explicit Command();

        const std::string& GetCommand() const;
        void SetCommand(const std::string& cmd);
        const std::string& GetDeviceId() const;
        void SetDeviceId(const std::string& dev);
        const Json::Value& GetParams() const;
        void SetParams(Json::Value params);
        const std::string& GetCreatedAt() const;
        void SetCreatedAt(const std::string& time);
        const std::string& GetUpdatedAt() const;
        void SetUpdatedAt(const std::string& time);
        const std::string& GetSentAt() const;
        void SetSentAt(const std::string& time);
        const std::string& GetAckAt() const;
        void SetAckAt(const std::string& time);
        const std::string& GetStatus() const;
        void SetStatus(const std::string& status);
        const std::string& GetError() const;
        void SetError(const std::string& error);

        const int GetAttempts() const;
        void SetAttempts(int attempts);
        const std::string& GetNextRetryAt() const;
        void SetNextRetryAt(const std::string& nextRetry);

        /**
         * Serialize object to JSON
         */
        Json::Value ToJson() const override;

        /**
         * Deserialize object from JSON
         */
        void FromJson(const Json::Value& json) override;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}  // namespace FalcataIoTServer