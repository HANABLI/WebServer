/**
 * @file Password.cpp
 * @brief
 * @copyright
 */

#include <Auth/Password.hpp>
#include <sodium.h>
#include <mutex>
#include <stdexcept>

namespace Auth
{
    bool SodiumInitOnce() {
        static std::once_flag once;
        static bool ok = false;
        std::call_once(once, []() { ok = (sodium_init() >= 0); });
        return ok;
    }

    std::string HashPasswordArgon2id(const std::string& password) {
        if (!SodiumInitOnce())
            throw std::runtime_error("sodium_init failed");

        char out[crypto_pwhash_STRBYTES];
        if (crypto_pwhash_str(out, password.c_str(), (unsigned long long)password.size(),
                              crypto_pwhash_OPSLIMIT_MODERATE,
                              crypto_pwhash_MEMLIMIT_MODERATE) != 0)
        { throw std::runtime_error("crypto_pwhash_str failed (OOM?)"); }
        return std::string(out);
    }

    bool VerifyPasswordArgon2id(const std::string& password, const std::string& hash) {
        if (!SodiumInitOnce())
            return false;
        if (hash.empty())
            return false;

        return crypto_pwhash_str_verify(hash.c_str(), password.c_str(),
                                        (unsigned long long)password.size()) == 0;
    }
}  // namespace Auth