# MakeLists.txt for WebServer
#
# © 2024 by Hatem Nabli


cmake_minimum_required(VERSION 3.8)
set(this WebServer)

set(Sources 
    src/main.cpp
)

add_executable(${this} ${Sources})
set_target_properties(${this} PROPERTIES 
    FOLDER Applications
)

target_link_libraries(${this} PUBLIC
    Http
    HttpNetworkTransport
    SystemUtils
)