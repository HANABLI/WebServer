# @file CMakeLists.txt for Repositories module
#
# © copyright 2025 by Hatem Nabli.
cmake_minimum_required(VERSION 3.20)
set(this Repositories)

set(Headers
    include/Repositories/GenericRepo.hpp
    include/Repositories/GenericRepo_impl.hpp
    include/Repositories/UserRepo.hpp
    include/Repositories/ServerRepo.hpp
    include/Repositories/SiteRepo.hpp
    include/Repositories/EventRepo.hpp
    include/Repositories/ZoneRepo.hpp
    include/Repositories/IotDevicesRepo.hpp
    include/Repositories/MqttTopicRepo.hpp

)

set(Sources
)

add_library(${this} STATIC ${Headers} ${Sources})
set_target_properties(${this} PROPERTIES
  FOLDER Libraries
)
if(MSVC)
    target_compile_options(${This} PRIVATE /EHsc)
else()
    target_compile_options(${This} PRIVATE -fexceptions)
endif()
target_link_libraries(${this} PUBLIC
    Models
    Factory
    PgClient
)

target_include_directories(${this} PUBLIC include)

