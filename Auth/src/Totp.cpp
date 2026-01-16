/**
 * @file Totp.cpp
 * @brief This is the implementation of Totp functions.
 * @copyright © copyright 2025 by Hatem Nabli.
 */
#include <Base32/Base32.hpp>
#include <Sha1/Sha1.hpp>
#include <Auth/Totp.hpp>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <sodium.h>
namespace
{
    static inline uint64_t ClampShiftedTime(uint64_t unixTime, int64_t deltaSeconds) {
        if (deltaSeconds < 0)
        {
            const uint64_t sub = (uint64_t)(-deltaSeconds);
            return (sub > unixTime) ? 0ULL : (unixTime - sub);
        }
        // deltaSeconds >= 0
        const uint64_t add = (uint64_t)deltaSeconds;
        // Saturation (très théorique)
        if (UINT64_MAX - unixTime < add)
            return UINT64_MAX;
        return unixTime + add;
    }

    static inline bool IsAllDigits(const std::string& s) {
        return std::all_of(s.begin(), s.end(),
                           [](unsigned char c) { return std::isdigit(c) != 0; });
    }

    static inline std::string OnlyDigits(const std::string& in) {
        std::string out;
        out.reserve(in.size());
        for (unsigned char ch : in)
            if (std::isdigit(ch))
                out.push_back((char)ch);
        return out;
    }

    static inline std::string ZeroPad(uint32_t v, int digits) {
        std::string s = std::to_string(v);
        if ((int)s.size() < digits)
            s.insert(s.begin(), (size_t)(digits - (int)s.size()), '0');
        return s;
    }

    static inline uint32_t Pow10i(int d) {
        uint32_t r = 1;
        for (int i = 0; i < d; ++i) r *= 10u;
        return r;
    }

    static inline std::vector<uint8_t> DecodeBase32Key(const std::string& secretBase32) {
        // Optionnel : nettoyage léger (espaces/tirets) si besoin
        std::string cleaned;
        cleaned.reserve(secretBase32.size());
        for (unsigned char c : secretBase32)
        {
            if (c == ' ' || c == '-' || c == '\t' || c == '\r' || c == '\n')
                continue;
            cleaned.push_back((char)c);
        }

        const std::string raw = Base32::Decode(cleaned);  // bytes (binaire) dans un string
        return std::vector<uint8_t>(raw.begin(), raw.end());
    }
}  // namespace

namespace Auth
{
    std::string TotpGenerateSecretBase32(size_t bytes) {
        if (!SodiumInitOnce())
            throw std::runtime_error("sodium_init failed");
        if (bytes < 10)
            bytes = 10;  // 80 bits mini (pratique); tu peux imposer 20 (160 bits) si tu veux.

        std::vector<uint8_t> raw(bytes);
        randombytes_buf(raw.data(), raw.size());

        // omitPadding=true -> pratique pour otpauth://
        return Base32::Encode(raw, /*omitPadding=*/true);
    }

    uint32_t TotpGenerateCode(const std::string& secretBase32, uint64_t unixTime, int digits,
                              int period) {
        if (digits < 6 || digits > 10)
            throw std::invalid_argument("digits out of range");
        if (period <= 0)
            throw std::invalid_argument("period must be > 0");

        const auto key = DecodeBase32Key(secretBase32);
        if (key.empty())
            throw std::runtime_error("empty TOTP key");

        const uint64_t counter = unixTime / (uint64_t)period;

        std::vector<uint8_t> msg(8);
        for (int i = 0; i < 8; ++i) msg[7 - i] = (uint8_t)(counter >> (i * 8));

        const auto mac = Sha1::HmacSha1(key, msg);  // 20 bytes attendu
        if (mac.size() < 20)
            throw std::runtime_error("HMAC-SHA1 invalid size");

        const int offset = mac[19] & 0x0F;
        if (offset + 3 >= (int)mac.size())
            throw std::runtime_error("HMAC offset out of range");

        const uint32_t bin =
            ((uint32_t)(mac[offset] & 0x7F) << 24) | ((uint32_t)(mac[offset + 1] & 0xFF) << 16) |
            ((uint32_t)(mac[offset + 2] & 0xFF) << 8) | ((uint32_t)(mac[offset + 3] & 0xFF));

        return bin % Pow10i(digits);
    }

    bool TotpVerify(const std::string& secretBase32, const std::string& code, uint64_t unixTime,
                    int digits, int period, int window) {
        if (digits < 6 || digits > 10)
            return false;
        if (period <= 0)
            return false;
        if (window < 0)
            window = 0;

        const std::string c = OnlyDigits(code);
        if ((int)c.size() != digits)
            return false;
        if (!IsAllDigits(c))
            return false;  // sécurité supplémentaire

        // Compare en constant-time sur des strings zero-padded
        for (int w = -window; w <= window; ++w)
        {
            const uint64_t t = ClampShiftedTime(unixTime, (int64_t)w * (int64_t)period);
            const uint32_t otp = TotpGenerateCode(secretBase32, t, digits, period);

            const std::string expected = ZeroPad(otp, digits);
            if (expected.size() == c.size() &&
                sodium_memcmp(expected.data(), c.data(), c.size()) == 0)
                return true;
        }
        return false;
    }
}  // namespace Auth