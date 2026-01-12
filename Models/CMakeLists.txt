# @file CMakeLists.txt for Model module
#
# © copyright 2025 by Hatem Nabli

cmake_minimum_required(VERSION 3.8)
set(this Models)

set(Headers
    include/Models/Core/IJsonSerialisable.hpp
    include/Models/Core/CoreObject.hpp
    include/Models/Core/Event.hpp
    include/Models/Core/Protocol.hpp
    include/Models/Core/Server.hpp
    include/Models/Core/IDevice.hpp
    include/Models/Core/Device.hpp
    include/Models/Core/IoTDevice.hpp
    include/Models/Auth/User.hpp
    include/Models/IoTDevices/MqttDevice.hpp
    include/Models/Data/MqttTopic.hpp
    include/Models/Servers/Modbus.hpp
    include/Models/Servers/MqttBroker.hpp
    include/Models/Servers/OpcUaServer.hpp
    include/Models/Location/Site.hpp
    include/Models/Location/Zone.hpp
)

set(Sources
    src/Core/CoreObject.cpp
    src/Core/Event.cpp
    src/Core/Server.cpp
    src/Core/IoTDevice.cpp
    src/Auth/User.cpp
    src/IoTDevices/MqttDevice.cpp
    src/Data/MqttTopic.cpp
    src/Servers/Modbus.cpp
    src/Servers/MqttBroker.cpp
    src/Servers/OpcUaServer.cpp
    src/Location/Site.cpp
    src/Location/Zone.cpp
)

add_library(${this} STATIC ${Headers} ${Sources})
set_target_properties(${this} PROPERTIES
    FOLDER Libraries
)
target_include_directories(${this} PUBLIC
    include
)

target_link_libraries(${this} PUBLIC
    SystemUtils
    StringUtils
    UuidV7
    Json
    MqttV5
    Utf8
    PgClient
    Auth
)