# CMakeLists.txt for ChatRoomPlugin
#
# © 2024 by Hatem Nabli

cmake_minimum_required(VERSION 3.8)
set(This ChatRoomPlugin)

set(Sources
    src/ChatRoomPlugin.cpp
)

add_library(${This} SHARED ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER "Plugins"
)

target_include_directories(${This} PRIVATE $<TARGET_PROPERTY:WebServer,INCLUDE_DIRECTORIES>)

target_link_libraries(${This} PUBLIC
    Uri
    Http
    Json
    SystemUtils
    StringUtils
    WebSocket
)

add_subdirectory(test)