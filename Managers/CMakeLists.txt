# @file CMakeLists for Managers module
#
# © 2025 by Hatem Nabli.

cmake_minimum_required(VERSION 3.20)
set(this Managers)

set(Headers
    include/Managers/UserManager.hpp
    include/Managers/DeviceManager.hpp
    include/Managers/DeviceRegistry.hpp
    include/Managers/MqttDeviceConnector.hpp
)

set(Sources
    src/UserManager.cpp
    src/DeviceManager.cpp
    src/DeviceRegistry.cpp
    src/MqttDeviceConnector.cpp
)

add_library(${this} STATIC ${Headers} ${Sources})
set_target_properties(${this} PROPERTIES FOLDER Libraries)

target_include_directories(${this} PUBLIC include)

target_link_libraries(${this} PUBLIC
    Models
    Auth
    Repositories
    Factory
    PgClient
    MqttV5
)