# @file AuthLoginPlugin.cpp
# @brief This is the build configuration of AuthLoginPlugin.
# @copyright © 2026 by Hatem Nabli.

cmake_minimum_required(VERSION 3.20)
set(This AuthLoginPlugin)

set(Sources
    src/AuthLoginPlugin.cpp
)

add_library(${This} SHARED ${Sources})

set_target_properties(${This} PROPERTIES FOLDER "Web Server Plugins")

target_include_directories(${This} PRIVATE $<TARGET_PROPERTY:WebServer,INCLUDE_DIRECTORIES> include)

target_link_libraries(
    ${This} PUBLIC Uri Http Json SystemUtils PgClient Models Managers Auth AuthService
)

