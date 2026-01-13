/**
 * @file SiteBuilder.hpp
 * @brief This is the declaration of the FalcataIoTServer::SiteBuilder
 * class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#pragma once

#include <Models/Location/Site.hpp>
#include <Factory/GenericFactory.hpp>

#include <memory>

namespace FalcataIoTServer
{
    class SiteBuilder
    {
    public:
        using Base = Site;

    public:
        static std::string Discriminator(const Postgresql::PgResult& res, int row);

        static std::unique_ptr<Base> Build(const std::string& kind, const Postgresql::PgResult& res,
                                           int row);
    };

    using SiteFactory = GenericFactory<SiteBuilder>;
}  // namespace FalcataIoTServer