#pragma once
/**
 * @file UserManager.hpp
 * @brief This is the declaration of the UserManager class.
 * @copyright © copyright 2026 by Hatem Nabli.
 */
#include <Models/Auth/User.hpp>
#include <Repositories/UserRepo.hpp>
#include <Repositories/GenericRepo.hpp>

#include <memory>
#include <string>
#include <vector>

namespace FalcataIoTServer
{
    class UserManager
    {
    public:
        ~UserManager() noexcept;
        UserManager(const UserManager&) = delete;
        UserManager(UserManager&&) noexcept = default;
        UserManager& operator=(const UserManager&) = delete;
        UserManager& operator=(UserManager&&) noexcept = default;

    public:
        explicit UserManager(std::shared_ptr<Postgresql::PgClient> pg);
        std::vector<std::unique_ptr<User>> ListUsers(const std::string& tenantId, int limit = 200);
        std::unique_ptr<User> GetUser(const std::string& tenantId, const std::string& userId);

        std::shared_ptr<User> SigninCreateUser(const std::string& tenanId,
                                               const std::string& userName,
                                               const std::string& password,
                                               const std::string& email, const std::string& role,
                                               bool mfaEnabled, int totpDigits, int totpPeriod);
        std::shared_ptr<User> SigninCreateUser(const Json::Value& object);
        std::unique_ptr<User> LoginVerify(const std::string& tenantId, const std::string& userName,
                                          const std::string& password, const std::string& totpCode);

        void UpdateUser(const std::shared_ptr<User>& u);
        void DeleteUser(const std::string& tenantId, const std::string& userId);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}  // namespace FalcataIoTServer