/**
 * @file AuthService.hpp
 * @brief This is the declaration of the FalcataIoTServer::AuthServiceHs256 class.
 * @copyright  © 2026 by Hatem Nabli
 */

#include <Auth/AuthService.hpp>
#include <Auth/Role.hpp>
#include <Auth/Jwt.hpp>
#include <Json/Json.hpp>
#include <memory>

namespace FalcataIoTServer
{
    class AuthServiceHs256 : public Auth::IAuthService
    {
    public:
        ~AuthServiceHs256() noexcept;
        AuthServiceHs256(const AuthServiceHs256&) = delete;
        AuthServiceHs256(AuthServiceHs256&&) noexcept = default;
        AuthServiceHs256& operator=(const AuthServiceHs256&) = delete;
        AuthServiceHs256& operator=(AuthServiceHs256&&) noexcept = default;

    public:
        explicit AuthServiceHs256(Json::Value cfg);

        std::optional<Auth::Identity> AthenticateBearer(
            const std::string& authorizationHeader) const override;

        bool Require(Auth::Role required, const std::string& authorizationHeader,
                     Auth::Identity* out = nullptr) const override;

        std::string IssueToken(const Auth::Identity& id, int ttlSeconds) const override;

    private:
        struct Impl;

        std::unique_ptr<Impl> impl_;
    };
}  // namespace FalcataIoTServer