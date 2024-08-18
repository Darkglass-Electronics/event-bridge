cmake_minimum_required(VERSION 3.15)
project(event-bridge)

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
pkg_check_modules(libinput IMPORTED_TARGET libinput)

#######################################################################################################################
# Setup event-bridge target

# check if we are building from this project, or are imported by another
if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)

  # building as regular application, uses Qt for event loop and notifies startup via systemd
  pkg_check_modules(systemd IMPORTED_TARGET libsystemd)

  find_package(Qt6 QUIET COMPONENTS Core Network SerialPort WebSockets)
  if (Qt6_FOUND)
    add_library(Qt::Core ALIAS Qt6::Core)
    add_library(Qt::Network ALIAS Qt6::Network)
    add_library(Qt::SerialPort ALIAS Qt6::SerialPort)
    add_library(Qt::WebSockets ALIAS Qt6::WebSockets)
  else()
    find_package(Qt5 REQUIRED COMPONENTS Core Network SerialPort WebSockets)
    add_library(Qt::Core ALIAS Qt5::Core)
    add_library(Qt::Network ALIAS Qt5::Network)
    add_library(Qt::SerialPort ALIAS Qt5::SerialPort)
    add_library(Qt::WebSockets ALIAS Qt5::WebSockets)
  endif()

  add_executable(event-bridge)

  set_target_properties(event-bridge
    PROPERTIES
      AUTOMOC ON
      AUTOUIC ON
      AUTORCC ON
  )

  target_compile_definitions(event-bridge
    PRIVATE
      $<$<BOOL:${libinput_FOUND}>:HAVE_LIBINPUT>
      $<$<BOOL:${systemd_FOUND}>:HAVE_SYSTEMD>
  )

  target_include_directories(event-bridge
    PRIVATE
      src
  )

  target_link_libraries(event-bridge
    PRIVATE
      $<$<BOOL:${libinput_FOUND}>:PkgConfig::libinput>
      $<$<BOOL:${systemd_FOUND}>:PkgConfig::systemd>
      Qt::Core
      Qt::Network
      Qt::SerialPort
      Qt::WebSockets
  )

  target_sources(event-bridge
    PRIVATE
      src/event-bridge.cpp
      src/events.cpp
      src/events-gpio.cpp
      src/main.cpp
      src/websocket.cpp
      $<$<BOOL:${libinput_FOUND}>:${PROJECT_SOURCE_DIR}/src/events-libinput.cpp>
  )

else()

  # building as interface library
  add_library(event-bridge INTERFACE)
  add_library(event::bridge ALIAS event-bridge)

  target_compile_definitions(event-bridge
    INTERFACE
      $<$<BOOL:${libinput_FOUND}>:HAVE_LIBINPUT>
  )

  target_include_directories(event-bridge
    INTERFACE
      src
  )

  target_link_libraries(event-bridge
    INTERFACE
      $<$<BOOL:${libinput_FOUND}>:PkgConfig::libinput>
  )

  target_sources(event-bridge
    INTERFACE
      src/event-bridge.cpp
      src/events.cpp
      src/events-gpio.cpp
      $<$<BOOL:${libinput_FOUND}>:${PROJECT_SOURCE_DIR}/src/events-libinput.cpp>
  )

endif()

#######################################################################################################################
