# @file CMakeLists Config file for Commands module
#
# copyright 2025 by Hatem Nabli

cmake_minimum_required(VERSION 3.8)
set(this Commands)

set(Headers
        include/Commands/Command.hpp
        include/Commands/CommandRepo.hpp
        include/Commands/CommandDispatcher.hpp
        )
set(Sources
        src/Command.cpp
        src/CommandRepo.cpp
        src/CommandDispatcher.cpp
)

add_library(${this} STATIC ${Headers} ${Sources})

set_target_properties(${this} PROPERTIES FOLDER Libraries)
target_include_directories(${this} PUBLIC include)

target_link_libraries(${this} PUBLIC
    SystemUtils
    Json
    WebSocket
    Models
    Managers
    PgClient
    StringUtils
)