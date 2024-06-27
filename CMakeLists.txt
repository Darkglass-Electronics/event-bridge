cmake_minimum_required(VERSION 3.15)
project(event-stream)

set(CMAKE_POLICY_DEFAULT_CMP0025 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0063 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

set_property(GLOBAL PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)

#######################################################################################################################
# Find deps

find_package(PkgConfig REQUIRED)
pkg_check_modules(libinput REQUIRED IMPORTED_TARGET libinput)
pkg_check_modules(systemd IMPORTED_TARGET libsystemd)

find_package(Qt6 QUIET COMPONENTS Core Network WebSockets)
if (Qt6_FOUND)
  add_library(Qt::Core ALIAS Qt6::Core)
  add_library(Qt::Network ALIAS Qt6::Network)
  add_library(Qt::WebSockets ALIAS Qt6::WebSockets)
else()
  find_package(Qt5 REQUIRED COMPONENTS Core Network WebSockets)
  add_library(Qt::Core ALIAS Qt5::Core)
  add_library(Qt::Network ALIAS Qt5::Network)
  add_library(Qt::WebSockets ALIAS Qt5::WebSockets)
endif()

#######################################################################################################################
# Setup connector target

add_executable(event-stream)

set_target_properties(event-stream
  PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_compile_definitions(event-stream
  PRIVATE
    $<$<BOOL:${systemd_FOUND}>:HAVE_SYSTEMD>
)

target_include_directories(event-stream
  PRIVATE
    src
)

target_link_libraries(event-stream
  PUBLIC
    PkgConfig::libinput
    $<$<BOOL:${systemd_FOUND}>:PkgConfig::systemd>
    Qt::Core
    Qt::Network
    Qt::WebSockets
)

target_sources(event-stream
  PRIVATE
    src/api.cpp
    src/input.cpp
    src/input-libinput.cpp
    src/main.cpp
    src/websocket.cpp
)

#######################################################################################################################
