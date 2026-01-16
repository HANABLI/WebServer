#pragma once
/**
 * @file Jwt.hpp
 * @brief This is the declaration of the Auth::VerifiedJwt structure and its functions.
 * @copyright copyright © 2025 by Hatem Nabli.
 */
#include <string>
#include <Base64/Base64.hpp>
#include <Json/Json.hpp>

namespace Auth
{
    struct VerifiedJwt
    {
        Json::Value header;
        Json::Value payload;
    };

    std::string MakeHs256(const Json::Value& header, const Json::Value& payload,
                          const std::string& secret);

    VerifiedJwt VerifyHs256(const std::string& token, const std::string& secret,
                            const std::string& iss = "", const std::string& aud = "");
    long NowEpoch();
}  // namespace Auth
