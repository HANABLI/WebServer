#pragma once
/**
 * @file TopologyUpdate.hpp
 * @brief This is the declaration of the FalcataIoTServer::TopologyUpdate class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */

#include <PgClient/PgClient.hpp>
#include <Topology/TopologyGraph.hpp>
#include <Managers/DeviceManager.hpp>
#include <WebSocket/WebSocket.hpp>
#include <memory>

namespace FalcataIoTServer
{
    class TopologyUpdater
    {
    public:
        ~TopologyUpdater() noexcept;
        TopologyUpdater(const TopologyUpdater&) = delete;
        TopologyUpdater(TopologyUpdater&&) noexcept = default;
        TopologyUpdater& operator=(const TopologyUpdater&) = delete;
        TopologyUpdater& operator=(TopologyUpdater&&) noexcept = default;

    public:
        explicit TopologyUpdater(std::shared_ptr<Postgresql::PgClient>& pg,
                                 std::shared_ptr<DeviceManager>& dm,
                                 std::shared_ptr<WebSocket::WebSocket>& ws);

        void Start();
        void Stop();

        const TopologyGraph& CurrentTopology() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}  // namespace FalcataIoTServer