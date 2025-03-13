# CMakeLists.txt for StaticCententPlugin
#
# © 2024 by Hatem Nabli

cmake_minimum_required(VERSION 3.8)
set(This StaticContentPlugin)

set(Sources
    src/StaticContentPlugin.cpp
)

add_library(${This} SHARED ${Sources})
set_target_properties(${This} PROPERTIES
    FOLDER "Web Server Plugins"
)

target_include_directories(${This} PRIVATE $<TARGET_PROPERTY:WebServer,INCLUDE_DIRECTORIES>)

target_link_libraries(${This} PUBLIC
    Uri
    Http
    Json
    SystemUtils
    StringUtils
)

add_subdirectory(test)