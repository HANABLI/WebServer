# MakeLists.txt for WebServer
#
# © 2024 by Hatem Nabli

cmake_minimum_required(VERSION 3.8)
set(this WebServer)

set(Headers
    include/WebServer/PluginEntryPoint.hpp
)

set(Sources
    src/main.cpp
    src/Plugin.hpp
    src/Plugin.cpp
    src/PluginLoader.hpp
    src/PluginLoader.cpp
    src/TimeKeeper.hpp
    src/TimeKeeper.cpp
)

add_executable(${this} ${Sources} ${Headers})
set_target_properties(${this} PROPERTIES
    FOLDER Applications
)

target_include_directories(${this} PUBLIC include)

target_link_libraries(${this} PUBLIC
    Json
    Http
    HttpNetworkTransport
    SystemUtils
    StringUtils
    WebSocket
    Sha1
)

add_subdirectory(StaticContentPlugin)
add_subdirectory(ChatRoomPlugin)
add_subdirectory(MqttPlugin)