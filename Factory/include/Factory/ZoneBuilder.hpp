/**
 * @file ZoneBuilder.hpp
 * @brief This is the declaration of the FalcataIoTServer::ZoneBuilder
 * class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#pragma once

#include <Models/Location/Zone.hpp>
#include <Factory/GenericFactory.hpp>

#include <memory>

namespace FalcataIoTServer
{
    class ZoneBuilder
    {
    public:
        using Base = Zone;

    public:
        static std::string Discriminator(const Postgresql::PgResult& res, int row);

        static std::unique_ptr<Base> Build(const std::string& siteId,
                                           const Postgresql::PgResult& res, int row);
    };

    using ZoneFactory = GenericFactory<ZoneBuilder>;
}  // namespace FalcataIoTServer