#pragma once
/**
 * @file Password.hpp
 * @brief
 * @copyright
 */

#include <string>

namespace Auth
{
    bool SodiumInitOnce();

    std::string HashPasswordArgon2id(const std::string& password);

    bool VerifyPasswordArgon2id(const std::string& password, const std::string& hash);

}  // namespace Auth