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
        void SetCommand(std::string& cmd);
        const std::string& GetDeviceId() const;
        void SetDeviceId(std::string& dev);
        const Json::Value& GetParams() const;
        void SetParams(Json::Value& params);

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