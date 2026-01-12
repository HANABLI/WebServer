#pragma once
/**
 * @file GenericFactory.hpp
 * @brief This is the definition of the GenericFactory template.
 * @copyright © copyright 2025 by Hatem Nabli
 */
#include <PgClient/PgResult.hpp>
#include <memory>

namespace FalcataIoTServer
{
    template <class Trait>
    class GenericFactory
    {
    public:
        using Base = typename Trait::Base;

        static std::unique_ptr<Base> FromRow(const Postgresql::PgResult& res, int row) {
            const std::string disc = Trait::Discriminator(res, row);
            return Trait::Build(disc, res, row);
        }

        static std::unique_ptr<Base> FromSingle(const Postgresql::PgResult& res) {
            if (res.Rows() != 1)
                throw std::runtime_error("GenericFactory::FromSingle: expected exactly one row");
            return FromRow(res, 0);
        }
    };

}  // namespace FalcataIoTServer