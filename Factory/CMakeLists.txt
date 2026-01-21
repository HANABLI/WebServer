# @file CMakeLists.txt for Factory
#
# © copyright 2025 by Hatem Nabli.
cmake_minimum_required(VERSION 3.20)
set(this Factory)

set(Headers
    include/Factory/GenericFactory.hpp
    include/Factory/UserBuilder.hpp
    include/Factory/ServerBuilder.hpp
    include/Factory/EventBuilder.hpp
    include/Factory/SiteBuilder.hpp
    include/Factory/ZoneBuilder.hpp
    include/Factory/MqttTopicBuilder.hpp
    include/Factory/IoTDeviceBuilder.hpp
)

set(Sources
    src/UserBuilder.cpp
    src/SiteBuilder.cpp
    src/ZoneBuilder.cpp
    src/ServerBuilder.cpp
    src/EventBuilder.cpp
    src/MqttTopicBuilder.cpp
    src/IoTDeviceBuilder.cpp
)

add_library(${this} STATIC ${Headers} ${Sources})
set_target_properties(${this} PROPERTIES
  FOLDER Libraries
)
target_link_libraries(${this} PUBLIC
    Models
    PgClient
)

target_include_directories(${this} PUBLIC include)

