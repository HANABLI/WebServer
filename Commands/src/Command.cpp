#pragma once
/**
 * @file Command.cpp
 * @brief This represent the implementation of the FalcataIoTServer::Command class.
 * @copyright  copyright © 2025 by Hatem Nabli.
 */
#include <Commands/Command.hpp>

namespace FalcataIoTServer
{
    struct Command::Impl
    {
        std::string deviceId;
        std::string createdAt;
        std::string updatedAt;
        std::string sentAt;
        std::string ackAt;

        std::string command;
        Json::Value params;

        std::string status;  // pending/sent/acked/failed/retry/cancelled
        std::string error;

        int attempts = 0;
        std::string nextRetryAt;
        explicit Impl();
        ~Impl() noexcept = default;
    };

    Command::Command() : CoreObject(), impl_(std::make_unique<Impl>()) {}
    Command::~Command() noexcept = default;

    const std::string& Command::GetCommand() const { return impl_->command; }
    void Command::SetCommand(std::string& cmd) { impl_->command = cmd; }
    const std::string& Command::GetDeviceId() const { return impl_->deviceId; }
    void Command::SetDeviceId(std::string& dev) { impl_->deviceId = dev; }
    const Json::Value& Command::GetParams() const { return impl_->params; }
    void Command::SetParams(Json::Value& params) { impl_->params = params; }

    Json::Value Command::ToJson() const {
        Json::Value command(Json::Value::Type::Object);
        command.Set("id", Uuid_s());
        command.Set("deviceId", impl_->deviceId);
        command.Set("createdAt", impl_->createdAt);
        command.Set("updatedAt", impl_->updatedAt);
        command.Set("sentAt", impl_->sentAt);
        command.Set("ackAt", impl_->ackAt);
        command.Set("command", impl_->command);
        command.Set("params", impl_->params);
        command.Set("status", impl_->status);
        command.Set("error", impl_->error);
        command.Set("attempts", impl_->attempts);
        command.Set("nextRetryAt", impl_->nextRetryAt);
        return command;
    }

    void Command::FromJson(const Json::Value& j) {
        if (j.Has("id"))
        { UuidFromString(j["id"]); }
        if (j.Has("deviceId"))
        { impl_->deviceId = j["deviceId"]; }
        if (j.Has("createdAt"))
        { impl_->createdAt = j["createdAt"]; }
        if (j.Has("updatedAt"))
        { impl_->createdAt = j["updatedAt"]; }
        if (j.Has("sentAt"))
        { impl_->sentAt = j["sentAt"]; }
        if (j.Has("ackAt"))
        { impl_->ackAt = j["ackAt"]; }
        if (j.Has("command"))
        { impl_->command = j["command"]; }
        if (j.Has("params"))
        { impl_->params = j["params"]; }
        if (j.Has("status"))
        { impl_->status = j["status"]; }
        if (j.Has("error"))
        { impl_->error = j["error"]; }
        if (j.Has("attempts"))
        { impl_->attempts = static_cast<int>(j["attempts"]); }
        if (j.Has("nextRetryAt"))
        { impl_->nextRetryAt = j["nextRetryAt"]; }
    }

}  // namespace FalcataIoTServer