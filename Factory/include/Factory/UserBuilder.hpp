/**
 * @file UserBuilder.hpp
 * @brief This is the declaration of the FalcataIoTServer::UserBuilder
 * class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#pragma once

#include <Models/Auth/User.hpp>
#include <Factory/GenericFactory.hpp>

#include <memory>

namespace FalcataIoTServer
{
    class UserBuilder
    {
    public:
        using Base = User;

    public:
        static std::string Discriminator(const Postgresql::PgResult& res, int row);

        static std::unique_ptr<Base> Build(const std::string& siteId,
                                           const Postgresql::PgResult& res, int row);
    };

    using UserFactory = GenericFactory<UserBuilder>;
}  // namespace FalcataIoTServer