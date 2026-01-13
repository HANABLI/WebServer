#pragma once
/**
 * @file UserRepo.hpp
 * @brief This is the declaration of the UserRepo class.
 * @copyright © 2025 by Hatem Nabli
 */
#include <Models/Auth/User.hpp>
#include <Repositories/GenericRepo.hpp>
#include <Factory/UserBuilder.hpp>
#include <memory>

namespace FalcataIoTServer
{
    struct UserRepo
    {
        using Factory = UserFactory;

        static std::string SelectAllSql() { return R"SQL(SELCET * FROM iot.users)SQL"; }

        static std::string ListSql() {
            return R"SQL(SELECT * FROM iot.users WHERE tenant_id=$1 ORDER BY created_at DESC LIMIT $2)SQL";
        }

        static std::string SelectByDisc() {
            return R"SQL(SELECT * FROM iot.users WHERE tenant_id=$1 AND user_name=$2 LIMIT 1)SQL";
        }

        static std::string SelectByIdsSql() {
            return R"SQL(SELECT * FROM iot.users WHERE tenant_id=$1 AND id=$2 LIMIT 1)SQL";
        }

        static std::string InsertSql() {
            return "INSERT INTO iot.users(id, tenant_id, user_name, email, password_hash, "
                   "mfa_enabled, totp_secret_b32, totp_digits, totp_period, role, disabled, "
                   "site_roles) "
                   "VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12::jsonb) RETURNING id";
        }

        static std::string UpdateSql() {
            return R"SQL("UPDATE iot.users SET email=$3, role=$4, disabled=$5, mfa_enabled=$6, totp_digits=$7, totp_period=$8 "
            "WHERE tenant_id=$1 AND id=$2")SQL";
        }

        static std::string DeleteSql() {
            return R"SQL("DELETE FROM iot.users WHERE tenant_id=$1 AND id=$2")SQL";
        }

        static std::string SetDisableSql() {
            return R"SQL("UPDATE iot.users SET disabled=$3 WHERE tenant_id=$1 AND id=$2")SQL";
        }
    };

    using UserRepository = GenericRepo<UserRepo>;
}  // namespace FalcataIoTServer
