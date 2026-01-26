# @file CMakeLists.txt for Auth module
#
# copyright 2025 by Hatem Nabli

cmake_minimum_required(VERSION 3.20)
set(this Auth)

set(Headers
    include/Auth/Role.hpp
    include/Auth/Password.hpp
    include/Auth/Guards.hpp
    include/Auth/Totp.hpp
    include/Auth/Jwt.hpp
    include/Auth/AuthService.hpp
    )

set(Sources
    src/Totp.cpp
    src/Jwt.cpp
    src/Guards.cpp
    src/Password.cpp
    src/AuthService.cpp)

add_library(${this} STATIC ${Headers} ${Sources})
set_target_properties(${this} PROPERTIES FOLDER Libraries)
# ---- libsodium (OS-aware) ----
# ---- libsodium ----
if(WIN32)
  # With vcpkg toolchain enabled, this will search vcpkg prefixes.
  find_path(LIBSODIUM_INCLUDE_DIR sodium.h REQUIRED)
  find_library(LIBSODIUM_LIBRARY NAMES sodium libsodium REQUIRED)

  message(STATUS "Found libsodium include: ${LIBSODIUM_INCLUDE_DIR}, lib: ${LIBSODIUM_LIBRARY}")
  target_include_directories(${this} PUBLIC ${LIBSODIUM_INCLUDE_DIR})
  target_link_libraries(${this} PUBLIC ${LIBSODIUM_LIBRARY})

else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBSODIUM REQUIRED libsodium)
  target_include_directories(${this} PUBLIC ${LIBSODIUM_INCLUDE_DIRS})
  target_link_libraries(${this} PUBLIC ${LIBSODIUM_LIBRARIES})
endif()

if(MSVC)
    target_compile_options(${this} PRIVATE /EHsc)
else()
    target_compile_options(${this} PRIVATE -fexceptions)
endif()

# find_path(LIBSODIUM_INCLUDE_DIR sodium.h
#   HINTS
#     "C:/Users/ADMIN/Documents/3_Dev/vcpkg/packages/libsodium_x64-windows/include"
# )

# find_library(LIBSODIUM_LIBRARY NAMES libsodium
#   HINTS
#     "C:/Users/ADMIN/Documents/3_Dev/vcpkg/packages/libsodium_x64-windows/lib"
# )

# if (LIBSODIUM_INCLUDE_DIR AND LIBSODIUM_LIBRARY)
#   message(STATUS "Found sodium include: ${LIBSODIUM_INCLUDE_DIR}, lib: ${LIBSODIUM_LIBRARY}")
#   target_include_directories(Auth PUBLIC ${LIBSODIUM_INCLUDE_DIR})
#   target_link_libraries(Auth PUBLIC ${LIBSODIUM_LIBRARY})
# else()
#   message(FATAL_ERROR "sodium (sodium.h / libsodium) not found. Use vcpkg to install libsodium.")
# endif()

target_include_directories(${this} PUBLIC include)

target_link_libraries(${this} PUBLIC
  Base32
  Base64
  Sha1
  Json
  Http
)