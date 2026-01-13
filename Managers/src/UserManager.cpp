/**
 * @file UserManager.cpp
 * @brief This is the implementation of the UserManager class.
 * @copyright © copyright 2026 by Hatem Nabli.
 */

#include <Managers/UserManager.hpp>
#include <Repositories/UserRepo.hpp>
#include <Auth/Password.hpp>
#include <Auth/Totp.hpp>
#include <Auth/Jwt.hpp>
#include <memory>

namespace FalcataIoTServer
{
    struct UserManager::Impl
    {
        std::shared_ptr<Postgresql::PgClient> pg;
        std::unique_ptr<UserRepository> repo;
        Impl(std::shared_ptr<Postgresql::PgClient> pg) : pg(pg) {}
        ~Impl() = default;
    };

    UserManager::UserManager(std::shared_ptr<Postgresql::PgClient> pg) :
        impl_(std::make_unique<Impl>(pg)) {
        impl_->repo = std::make_unique<UserRepository>(pg);
    }
    UserManager::~UserManager() noexcept = default;

    std::vector<std::unique_ptr<User>> UserManager::ListUsers(const std::string& tenantId,
                                                              int limit) {
        std::vector<std::string> params;
        params.push_back(tenantId);
        params.push_back(std::to_string(limit));
        return impl_->repo->List(params);
    }

    std::unique_ptr<User> UserManager::GetUser(const std::string& tenantId,
                                               const std::string& userId) {
        std::vector<std::string> params;
        params.push_back(tenantId);
        params.push_back(userId);
        return impl_->repo->FindByIds(params);
    }

    std::shared_ptr<User> UserManager::SigninCreateUser(const std::string& tenanId,
                                                        const std::string& userName,
                                                        const std::string& password,
                                                        const std::string& email,
                                                        const std::string& role, bool mfaEnabled,
                                                        int totpDigits, int totpPeriod) {
        auto user = std::make_shared<User>();
        user->SetTenantId(tenanId);
        user->SetUsername(userName);
        user->SetPasswordHash(Auth::HashPasswordArgon2id(password));
        user->SetEmail(email);
        user->SetRole(role);
        user->SetMfaEnabled(mfaEnabled);
        user->SetTotpDigits(totpDigits);
        user->SetTotpPeriod(totpPeriod);
        if (mfaEnabled)
        { user->SetMfaSecretB32(Auth::TotpGenerateSecretBase32(20)); }
        if (impl_->repo->Insert(user) == user->Uuid_s())
        { return user; }
        return nullptr;
    }

    std::shared_ptr<User> UserManager::SigninCreateUser(const Json::Value& object) {
        auto user = std::make_shared<User>(true, true);
        user->FromJson(object);
        if (impl_->repo->Insert(user) == user->Uuid_s())
        { return user; }
        return nullptr;
    }

    std::unique_ptr<User> UserManager::LoginVerify(const std::string& tenantId,
                                                   const std::string& userName,
                                                   const std::string& password,
                                                   const std::string& totpCode) {
        std::vector<std::string> params;
        params.push_back(tenantId);
        params.push_back(userName);
        auto u = impl_->repo->FindByDiscriminator(params);
        if (!u->IsEnabled())
        { throw std::runtime_error("user disabled"); }

        if (!Auth::VerifyPasswordArgon2id(password, u->GetPasswordHash()))
        { throw std::runtime_error("bad credentials"); }

        if (u->IsMfaEnabled())
        {
            if (u->GetMfaSecretB32().empty())
                throw std::runtime_error("mfa misconfiguration");
            if (totpCode.empty())
                throw std::runtime_error("mfa required");
            const uint64_t now = (uint64_t)Auth::NowEpoch();
            if (!Auth::TotpVerify(u->GetMfaSecretB32(), totpCode, now, u->GetTotpDigits(),
                                  u->GetTotpPeriod()))
            { throw std::runtime_error("bad totp"); }
        }
        return u;
    }

    void UserManager::UpdateUser(const std::shared_ptr<User>& u) { impl_->repo->Update(u); }
    void UserManager::DeleteUser(const std::string& tenantId, const std::string& userId) {
        std::vector<std::string> params;
        params.push_back(tenantId);
        params.push_back(userId);
        impl_->repo->Remove(params);
    }
}  // namespace FalcataIoTServer
