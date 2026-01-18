#pragma once
/**
 * @file CommandRepo.hpp
 * @brief This module contains the definition of the CommandRepo class.
 * @copyright © 2025 by Hatem Nabli.
 */
#include <vector>
#include <optional>
#include <functional>
#include <Commands/Command.hpp>
#include <PgClient/PgClient.hpp>
#include <PgClient/PgResult.hpp>

namespace FalcataIoTServer
{
    class CommandRepo
    {
    public:
        ~CommandRepo() noexcept = default;
        CommandRepo(const CommandRepo&) = delete;
        CommandRepo(CommandRepo&&) noexcept = default;
        CommandRepo& operator=(const CommandRepo&) = delete;
        CommandRepo& operator=(CommandRepo&&) noexcept = default;

    public:
        explicit CommandRepo(std::shared_ptr<Postgresql::PgClient> pg);

        std::vector<std::shared_ptr<Command>> FetchPending(int limit = 50);
        std::string InsertPending(const std::string& deviceId, const std::string& command,
                                  const Json::Value& params);
        std::shared_ptr<Command> GetById(const std::string& id);
        void Listen(const std::string& channel, const std::function<void()>& fn);
        void MarkSent(const std::string& id);
        void MarkAcked(const std::string& id);
        void MarkFailed(const std::string& id, const std::string& err);
        void ScheduleRetry(const std::string& id, int attempts, int delaySec,
                           const std::string& err);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

}  // namespace FalcataIoTServer
