# @file CMakelists.txt build configuration for AuthService module.
#
#  @copyright © 2026 by Hatem Nabli.

cmake_minimum_required(VERSION 3.8)
set(This AuthService)


set(Headers
    include/AuthService/AuthService.hpp
)

set(Sources
    src/AuthService.cpp
)

add_library(${This} STATIC ${Headers} ${Sources})

set_target_properties(${This} PROPERTIES FOLDER Libraries)

target_include_directories(${This} PUBLIC include)

target_link_libraries(${This} PUBLIC Auth Json)