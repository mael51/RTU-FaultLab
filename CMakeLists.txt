cmake_minimum_required(VERSION 3.16)
project(RTU-FaultLab LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RTU_ENABLE_SANITIZERS "Enable ASan/UBSan" OFF)
option(RTU_ENABLE_COVERAGE "Enable code coverage (gcov)" OFF)

if(RTU_ENABLE_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

if(RTU_ENABLE_COVERAGE)
  add_compile_options(-fprofile-arcs -ftest-coverage -O0 -g)
  add_link_options(-fprofile-arcs -ftest-coverage)
endif()

# Warnings
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# GoogleTest via FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows/MinGW, avoid overriding CRT
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Core library (empty for now, grows phase by phase)
add_library(rtufaultlab_core STATIC
  firmware/core/core_placeholder.cpp
  firmware/core/modbus_crc.cpp
  firmware/core/modbus_parser.cpp
  firmware/core/modbus_handler.cpp
  firmware/core/register_map.cpp
  firmware/core/safety_controller.cpp
)
target_include_directories(rtufaultlab_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Host adapter library (empty for now)
add_library(rtufaultlab_host STATIC
  firmware/host/host_placeholder.cpp
  firmware/host/clock_host.cpp
  firmware/host/gpio_mock.cpp
  firmware/host/transport_loopback.cpp
  firmware/host/serial_transport.cpp
)
target_link_libraries(rtufaultlab_host PUBLIC rtufaultlab_core)
target_include_directories(rtufaultlab_host PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Tests
enable_testing()
add_executable(rtufaultlab_tests
  tests/unit/test_phase0.cpp
  tests/unit/test_registers_phase1.cpp
  tests/unit/test_modbus_crc.cpp
  tests/unit/test_modbus_parser.cpp
  tests/unit/test_registers.cpp
  tests/unit/test_safety.cpp
  tests/unit/test_ipc.cpp
)

# Host DUT process (SIL)
add_executable(rtufaultlab_dut_host
  firmware/host/dut_host_main.cpp
)
target_link_libraries(rtufaultlab_dut_host PRIVATE rtufaultlab_core rtufaultlab_host)
target_link_libraries(rtufaultlab_tests PRIVATE rtufaultlab_core rtufaultlab_host GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(rtufaultlab_tests)