#pragma once
/**
 * @file CommandDispatcher.hpp
 * @brief This module contains the declaration of the FalcataIoTServer::CommandDispatcher class.
 * @copyright © 2025 by Hatem Nabli.
 */

#include <Commands/CommandRepo.hpp>
#include <Managers/DeviceManager.hpp>
#include <WebSocket/WebSocket.hpp>
#include <memory>

namespace FalcataIoTServer
{
    class CommandDispatcher
    {
    public:
        ~CommandDispatcher() noexcept;
        CommandDispatcher(const CommandDispatcher&) = delete;
        CommandDispatcher(CommandDispatcher&&) noexcept = default;
        CommandDispatcher& operator=(const CommandDispatcher&) = delete;
        CommandDispatcher& operator=(CommandDispatcher&&) noexcept = default;

    public:
        explicit CommandDispatcher(std::shared_ptr<CommandRepo>& repo,
                                   std::shared_ptr<DeviceManager>& dm);

        void Start();
        void Stop();

        void OnDbSignal();

        void OnMqttMessage(const std::string& topic, const std::string& payload);

        void SetWsBroadcaster(std::function<void(const std::string& jsonText)> wsFn);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}  // namespace FalcataIoTServer