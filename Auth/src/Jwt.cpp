// Auth/Jwt.cpp
#include <Auth/Jwt.hpp>
#include <Auth/Password.hpp>

#include <sodium.h>
#include <stdexcept>
#include <vector>
#include <ctime>

namespace
{
    std::string hmac_sha256(const std::string& key, const std::string& msg) {
        unsigned char out[crypto_auth_hmacsha256_BYTES];
        crypto_auth_hmacsha256_state st;
        crypto_auth_hmacsha256_init(&st, (const unsigned char*)key.data(), key.size());
        crypto_auth_hmacsha256_update(&st, (const unsigned char*)msg.data(), msg.size());
        crypto_auth_hmacsha256_final(&st, out);
        return std::string((const char*)out, sizeof(out));
    }

    void constant_time_eq(const std::string& a, const std::string& b) {
        if (a.size() != b.size())
            throw std::runtime_error("sig size mismatch");
        if (sodium_memcmp(a.data(), b.data(), a.size()) != 0)
            throw std::runtime_error("bad signature");
    }
}  // namespace

namespace Auth
{
    long NowEpoch() { return (long)std::time(nullptr); }

    std::string MakeHs256(const Json::Value& header, const Json::Value& payload,
                          const std::string& secret) {
        if (!SodiumInitOnce())
            throw std::runtime_error("sodium_init failed");
        const std::string h = header.ToEncoding();
        const std::string p = payload.ToEncoding();
        const std::string h64 = Base64::EncodeToBase64(h);
        const std::string p64 = Base64::EncodeToBase64(p);
        const std::string signingInput = h64 + "." + p64;
        const std::string sig = hmac_sha256(secret, signingInput);
        const std::string s64 = Base64::EncodeToBase64(sig);
        return signingInput + "." + s64;
    }

    VerifiedJwt VerifyHs256(const std::string& token, const std::string& secret,
                            const std::string& iss, const std::string& aud) {
        if (!SodiumInitOnce())
            throw std::runtime_error("sodium_init failed");

        const auto p1 = token.find('.');
        const auto p2 = token.find('.', p1 == std::string::npos ? p1 : p1 + 1);
        if (p1 == std::string::npos || p2 == std::string::npos)
            throw std::runtime_error("bad jwt format");

        const std::string h64 = token.substr(0, p1);
        const std::string p64 = token.substr(p1 + 1, p2 - p1 - 1);
        const std::string s64 = token.substr(p2 + 1);

        const std::string signingInput = h64 + "." + p64;
        const std::string expect = hmac_sha256(secret, signingInput);
        const std::string got = Base64::DecodeFromBase64(s64);
        constant_time_eq(expect, got);

        VerifiedJwt v;
        v.header = Json::Value::FromEncoding(Base64::DecodeFromBase64(h64));
        v.payload = Json::Value::FromEncoding(Base64::DecodeFromBase64(p64));

        // standard claims checks (exp/nbf/iat minimal)
        const long now = NowEpoch();
        if (v.payload.Has("exp") && (double)v.payload["exp"] < now)
            throw std::runtime_error("jwt expired");
        if (v.payload.Has("nbf") && (double)v.payload["nbf"] > now)
            throw std::runtime_error("jwt not active");
        if (!iss.empty() && v.payload.Has("iss") && v.payload["iss"].ToEncoding() != iss)
            throw std::runtime_error("bad iss");
        if (!aud.empty() && v.payload.Has("aud") && v.payload["aud"].ToEncoding() != aud)
            throw std::runtime_error("bad aud");

        return v;
    }
}  // namespace Auth
