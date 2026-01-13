# @file CMakeLists.txt for Topology module.
#
# copyright 2025 by Hatem Nabli.
cmake_minimum_required(VERSION 3.8)
set(this Topology)

set(Headers
    include/Topology/TopologyGraph.hpp
    include/Topology/TopologyUpdate.hpp
)

set(Sources
    src/TopologyGraph.cpp
    src/TopologyUpdate.cpp
)

add_library(${this} STATIC ${Headers} ${Sources})
set_target_properties(${this} PROPERTIES
  FOLDER Libraries
)
target_include_directories(${this} PUBLIC include)

target_link_libraries(${this} PUBLIC
    Models
    Managers
    PgClient
    Json
    WebSocket
    SystemUtils
)