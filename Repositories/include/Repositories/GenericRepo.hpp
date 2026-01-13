#pragma once

/**
 * @file GenericRepo.hpp
 */
#include <PgClient/PgClient.hpp>
#include <PgClient/PgResult.hpp>
#include <Factory/ServerBuilder.hpp>

#include <memory>
#include <vector>
namespace FalcataIoTServer
{
    template <class RepoTrait>
    class GenericRepo
    {
    public:
        using Factory = typename RepoTrait::Factory;
        using Base = typename Factory::Base;
        // Life cycle managment
    public:
        ~GenericRepo();
        GenericRepo(const GenericRepo&) = delete;
        GenericRepo(GenericRepo&&) noexcept = default;
        GenericRepo& operator=(const GenericRepo&) = delete;
        GenericRepo& operator=(GenericRepo&&) noexcept = default;
        // Methods
    public:
        explicit GenericRepo(std::shared_ptr<Postgresql::PgClient> client);

        /**
         *
         */
        std::vector<std::unique_ptr<Base>> FindAll();

        /**
         *
         */
        std::vector<std::unique_ptr<Base>> List(std::vector<std::string>& params);

        /**
         *
         */
        std::unique_ptr<Base> FindById(const std::string& id);

        /**
         *
         */
        std::unique_ptr<Base> FindByIds(const std::vector<std::string>& ids);

        /**
         *
         */
        std::unique_ptr<Base> FindByDiscriminator(const std::vector<std::string>& disc);

        /**
         *
         */
        const std::string& Insert(const std::shared_ptr<Base>& base);

        /**
         *
         */
        void Update(const std::shared_ptr<Base>& base);

        /**
         *
         */
        void Remove(const std::shared_ptr<Base>& base);

        /**
         *
         */
        void Remove(const std::vector<std::string>& params);

        /**
         *
         */
        void SetDisabled(const std::shared_ptr<Base>& base, bool disabled);

    private:
        struct Impl;

        std::unique_ptr<Impl> impl_;
    };
}  // namespace FalcataIoTServer

#define REPOSITORIES_GENERICREPO_HPP_INCLUDED
#include "GenericRepo_impl.hpp"
#undef REPOSITORIES_GENERICREPO_HPP_INCLUDED