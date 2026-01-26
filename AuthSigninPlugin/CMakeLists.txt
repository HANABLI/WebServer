# @file CMakeLists.txt file for AuthSigninPlugin module.
#
# @copyright © 2026 by Hatem Nabli.

cmake_minimum_required(VERSION 3.20)
set(This AuthSigninPlugin)


set(Sources
    src/AuthSigninPlugin.cpp
)

add_library(${This} SHARED ${Sources})

set_target_properties(${This} PROPERTIES
    FOLDER "Web Server Plugins"
)
if(MSVC)
    target_compile_options(${This} PRIVATE /EHsc)
else()
    target_compile_options(${This} PRIVATE -fexceptions)
endif()
target_include_directories(${This} PRIVATE $<TARGET_PROPERTY:WebServer,INCLUDE_DIRECTORIES> include)


target_link_libraries(${This} PUBLIC
    Uri Http Json SystemUtils StringUtils PgClient Models Managers Auth AuthService
)
