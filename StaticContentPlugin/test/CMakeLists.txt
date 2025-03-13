# CMakeLists.txt for UriTests
#
# © 2024 by Hatem Nabli

cmake_minimum_required(VERSION 3.8)
set(this StaticContentPluginTests)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:StaticContentPlugin>)

set(Sources
    src/StaticContentPluginTests.cpp
)

add_executable(${this} ${Sources})
set_target_properties(${this} PROPERTIES
    FOLDER Tests
)

target_include_directories(${this} PRIVATE ..)
target_include_directories(${this} PRIVATE $<TARGET_PROPERTY:WebServer,INCLUDE_DIRECTORIES>)

target_link_libraries(${this} PUBLIC
    gtest_main
    StaticContentPlugin
)

# add_custom_command(TARGET ${this} POST_BUILD
# COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:StaticContentPlugin> $<TARGET_FILE_DIR:${This}>
# )
add_test(
    NAME ${this}
    COMMAND ${this}
)