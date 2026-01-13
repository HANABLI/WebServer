#pragma once

#ifndef REPOSITORIES_GENERICREPO_HPP_INCLUDED
#    error "GenericRepo_impl.hpp must be included only via GenericRepo.hpp"
#endif

#include <Repositories/GenericRepo.hpp>
#include <PgClient/PgClient.hpp>
#include <PgClient/PgResult.hpp>
#include <memory>
#include <stdexcept>
#include <vector>
#include <string>

namespace FalcataIoTServer
{
    template <class RepoTrait>
    struct GenericRepo<RepoTrait>::Impl
    { std::shared_ptr<Postgresql::PgClient> pgclient; };

    template <class RepoTrait>
    GenericRepo<RepoTrait>::~GenericRepo() = default;

    template <class RepoTrait>
    GenericRepo<RepoTrait>::GenericRepo(std::shared_ptr<Postgresql::PgClient> client) :
        impl_(std::make_unique<Impl>()) {
        impl_->pgclient = std::move(client);
    }

    template <class RepoTrait>
    std::vector<std::unique_ptr<typename GenericRepo<RepoTrait>::Base>>
    GenericRepo<RepoTrait>::FindAll() {
        Postgresql::PgResult res(impl_->pgclient->Exec(RepoTrait::SelectAllSql()));
        using Factory = typename GenericRepo<RepoTrait>::Factory;
        using Base = typename GenericRepo<RepoTrait>::Base;

        std::vector<std::unique_ptr<Base>> out;
        out.reserve(static_cast<size_t>(res.Rows()));
        for (int r = 0; r < res.Rows(); ++r)
        { out.emplace_back(Factory::FromRow(res, r)); }
        return out;
    }

    template <class RepoTrait>
    std::vector<std::unique_ptr<typename GenericRepo<RepoTrait>::Base>>
    GenericRepo<RepoTrait>::List(std::vector<std::string>& params) {
        Postgresql::PgResult res(impl_->pgclient->ExecParams(RepoTrait::ListSql(), params));
        using Factory = typename GenericRepo<RepoTrait>::Factory;
        using Base = typename GenericRepo<RepoTrait>::Base;

        if (res.Rows() == 0)
            return {};
        std::vector<std::unique_ptr<Base>> out;
        out.reserve(static_cast<size_t>(res.Rows()));
        for (int r = 0; r < res.Rows(); ++r)
        { out.emplace_back(Factory::FromRow(res, r)); }
        return out;
    }

    template <class RepoTrait>
    std::unique_ptr<typename GenericRepo<RepoTrait>::Base> GenericRepo<RepoTrait>::FindById(
        const std::string& id) {
        Postgresql::PgResult res(impl_->pgclient->ExecParams(RepoTrait::SelectByIdSql(), {id}));
        if (res.Rows() == 0)
            return nullptr;
        if (res.Rows() != 1)
            throw std::runtime_error("FindById: expected 1 row.");
        using Factory = typename GenericRepo<RepoTrait>::Factory;
        return Factory::FromSingle(res);
    }

    template <class RepoTrait>
    std::unique_ptr<typename GenericRepo<RepoTrait>::Base> GenericRepo<RepoTrait>::FindByIds(
        const std::vector<std::string>& ids) {
        Postgresql::PgResult res(impl_->pgclient->ExecParams(RepoTrait::SelectByIdsSql(), ids));
        if (res.Rows() == 0)
            return nullptr;
        if (res.Rows() != 1)
            throw std::runtime_error("FindByIds: expected 1 row.");
        using Factory = typename GenericRepo<RepoTrait>::Factory;
        return Factory::FromSingle(res);
    }

    template <class RepoTrait>
    std::unique_ptr<typename GenericRepo<RepoTrait>::Base>
    GenericRepo<RepoTrait>::FindByDiscriminator(const std::vector<std::string>& disc) {
        Postgresql::PgResult res(impl_->pgclient->ExecParams(RepoTrait::SelectByDisc(), disc));
        if (res.Rows() == 0)
            return nullptr;
        if (res.Rows() != 1)
            throw std::runtime_error("FindByDiscriminator: expected 1 row.");
        using Factory = typename GenericRepo<RepoTrait>::Factory;
        return Factory::FromSingle(res);
    }

    template <class RepoTrait>
    const std::string& GenericRepo<RepoTrait>::Insert(const std::shared_ptr<Base>& base) {
        Postgresql::PgResult res(
            impl_->pgclient->ExecParams(RepoTrait::InsertSql(), base->GetInsertParams()));
        if (res.Rows() != 1)
            throw std::runtime_error("Insert: expected 1 row.");
        static thread_local std::string id;
        id = res.TextRequired(0, "id");
        return id;
    }

    template <class RepoTrait>
    void GenericRepo<RepoTrait>::Update(const std::shared_ptr<Base>& base) {
        Postgresql::PgResult res(
            impl_->pgclient->ExecParams(RepoTrait::UpdateSql(), base->GetUpdateParams()));
        if (res.Status() != Postgresql::PgStatus::CommandOk)
            throw std::runtime_error("Update: failed.");
    }

    template <class RepoTrait>
    void GenericRepo<RepoTrait>::Remove(const std::shared_ptr<Base>& base) {
        Postgresql::PgResult res(
            impl_->pgclient->ExecParams(RepoTrait::DeleteSql(), base->GetRemoveParams()));
        if (res.Status() != Postgresql::PgStatus::CommandOk)
            throw std::runtime_error("Remove: failed.");
    }

    template <class RepoTrait>
    void GenericRepo<RepoTrait>::Remove(const std::vector<std::string>& params) {
        Postgresql::PgResult res(impl_->pgclient->ExecParams(RepoTrait::DeleteSql(), params));
        if (res.Status() != Postgresql::PgStatus::CommandOk)
            throw std::runtime_error("Remove: failed.");
    }

    template <class RepoTrait>
    void GenericRepo<RepoTrait>::SetDisabled(const std::shared_ptr<Base>& base, bool disabled) {
        auto params = base->GetDisableParams();
        params.push_back(disabled ? "true" : "false");
        Postgresql::PgResult res(impl_->pgclient->ExecParams(RepoTrait::SetDisableSql(), params));
        if (res.Status() != Postgresql::PgStatus::CommandOk)
            throw std::runtime_error("SetDisabled: failed.");
    }
}  // namespace FalcataIoTServer
