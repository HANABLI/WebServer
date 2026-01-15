#pragma once
/**
 * @file Role.hpp
 * @brief
 * @copyright
 */

#include <string>
#include <algorithm>

namespace Auth
{
    enum class Role
    {
        Viewer = 0,
        Operator = 1,
        Admin = 2
    };

    inline std::string ToString(Role r) {
        switch (r)
        {
        case Role::Admin:
            return "admin";
        case Role::Operator:
            return "operator";
        default:
            return "viewer";
        }
    }

    inline Role ParseRole(const std::string& s) {
        auto x = s;
        std::transform(x.begin(), x.end(), x.begin(), ::tolower);
        if (x == "admin")
            return Role::Admin;
        if (x == "operator")
            return Role::Operator;
        return Role::Viewer;
    }

    inline bool HasAtLeast(Role role, Role required) { return (int)role >= (int)required; }
}  // namespace Auth