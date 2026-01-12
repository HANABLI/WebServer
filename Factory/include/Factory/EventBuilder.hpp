/**
 * @file SiteBuilder.hpp
 * @brief This is the definition of the FalcataIotServer::SiteBuilder
 * class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#pragma once

#include <Models/Core/Event.hpp>
#include <Factory/GenericFactory.hpp>

#include <memory>

namespace FalcataIoTServer
{
    class EventBuilder
    {
    public:
        using Base = Event;

    public:
        static std::string Discriminator(const Postgresql::PgResult& res, int row);

        static std::unique_ptr<Base> Build(const std::string& siteId,
                                           const Postgresql::PgResult& res, int row);
    };

    using EventFactory = GenericFactory<EventBuilder>;
}  // namespace FalcataIoTServer