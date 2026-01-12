/**
 * @file UserBuilder.cpp
 * @brief This is the implementation of the UserBuilder class.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#include <Factory/UserBuilder.hpp>

namespace FalcataIoTServer
{
    std::string UserBuilder::Discriminator(const Postgresql::PgResult& res, int row) {
        return res.TextRequired(row, "role");
    }

    std::unique_ptr<UserBuilder::Base> UserBuilder::Build(const std::string&,
                                                          const Postgresql::PgResult& res,
                                                          int row) {
        auto u = User::FromRow(res, row);
        return u;
    }
}  // namespace FalcataIoTServer