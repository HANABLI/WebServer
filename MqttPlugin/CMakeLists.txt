# CMakeLists.txt for ChatRoomPlugin
#
# © 2024 by Hatem Nabli

cmake_minimum_required(VERSION 3.20)
set(This MqttClientPlugin)

set(Sources
    src/TimeKeeper.hpp
    src/TimeKeeper.cpp
    src/MqttClientPlugin.cpp
)

add_library(${This} SHARED ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER "Web Server Plugins"
)

target_include_directories(${This} PRIVATE $<TARGET_PROPERTY:WebServer,INCLUDE_DIRECTORIES>)

target_link_libraries(${This} PUBLIC
    Uri
    Http
    MqttV5
    MqttNetworkTransport
    Json
    SystemUtils
    StringUtils
    WebSocket
)

#add_subdirectory(test)