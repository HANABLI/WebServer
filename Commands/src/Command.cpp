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
    void Command::SetCommand(const std::string& cmd) { impl_->command = cmd; }
    const std::string& Command::GetDeviceId() const { return impl_->deviceId; }
    void Command::SetDeviceId(const std::string& dev) { impl_->deviceId = dev; }
    const Json::Value& Command::GetParams() const { return impl_->params; }
    void Command::SetParams(Json::Value params) { impl_->params = params; }

    const std::string& Command::GetCreatedAt() const { return impl_->createdAt; }
    void Command::SetCreatedAt(const std::string& time) { impl_->createdAt = time; }
    const std::string& Command::GetUpdatedAt() const { return impl_->updatedAt; }
    void Command::SetUpdatedAt(const std::string& time) { impl_->updatedAt = time; }
    const std::string& Command::GetSentAt() const { return impl_->sentAt; }
    void Command::SetSentAt(const std::string& time) { impl_->sentAt = time; }
    const std::string& Command::GetAckAt() const { return impl_->ackAt; }
    void Command::SetAckAt(const std::string& time) { impl_->ackAt = time; }
    const std::string& Command::GetStatus() const { return impl_->status; }
    void Command::SetStatus(const std::string& status) { impl_->status = status; }
    const std::string& Command::GetError() const { return impl_->error; }
    void Command::SetError(const std::string& error) { impl_->error = error; }

    const int Command::GetAttempts() const { return impl_->attempts; }
    void Command::SetAttempts(int attempts) { impl_->attempts = attempts; }
    const std::string& Command::GetNextRetryAt() const { return impl_->nextRetryAt; }
    void Command::SetNextRetryAt(const std::string& nextRetry) { impl_->nextRetryAt = nextRetry; }

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
        { SetDeviceId(j["deviceId"]); }
        if (j.Has("createdAt"))
        { SetCreatedAt(j["createdAt"]); }
        if (j.Has("updatedAt"))
        { SetUpdatedAt(j["updatedAt"]); }
        if (j.Has("sentAt"))
        { SetSentAt(j["sentAt"]); }
        if (j.Has("ackAt"))
        { SetAckAt(j["ackAt"]); }
        if (j.Has("command"))
        { SetCommand(j["command"]); }
        if (j.Has("params"))
        { SetParams(j["params"]); }
        if (j.Has("status"))
        { SetStatus(j["status"]); }
        if (j.Has("error"))
        { SetError(j["error"]); }
        if (j.Has("attempts"))
        { SetAttempts(static_cast<int>(j["attempts"])); }
        if (j.Has("nextRetryAt"))
        { SetNextRetryAt(j["nextRetryAt"]); }
    }

}  // namespace FalcataIoTServer