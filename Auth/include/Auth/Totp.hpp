#pragma once
/**
 * @file Totp.hpp
 * @brief This is the declaration of Mfa functions allowing the generation and verification of totp
 * token.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#include <Auth/Password.hpp>
#include <string>
#include <cstdint>

namespace Auth
{
    std::string TotpGenerateSecretBase32(size_t bytes = 20);

    uint32_t TotpGenerateCode(const std::string& secretBase32, uint64_t unixTime, int digits = 6,
                              int period = 30);

    bool TotpVerify(const std::string& secretBase32, const std::string& code, uint64_t unixTime,
                    int digits = 6, int period = 30, int window = 1);
}  // namespace Auth