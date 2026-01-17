/**
 * @file AuthService.hpp
 * @brief This represent the implementation of the Auth interface.
 * @copyright copyright © 2025 by Hatem Nabli.
 */
#include <Auth/AuthService.hpp>
#include <memory>

namespace Auth
{
    static std::shared_ptr<IAuthService> g = nullptr;
    void Set(std::shared_ptr<IAuthService> svc) { g = svc; }
    std::shared_ptr<IAuthService> Get() { return g; }
}  // namespace Auth