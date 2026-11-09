cmake_minimum_required(VERSION 3.16)
project(gridflow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Threads REQUIRED)

# Shared include paths and warnings. Real source files join this later.
add_library(gridflow_core INTERFACE)
target_include_directories(gridflow_core INTERFACE include third_party)
target_compile_options(gridflow_core INTERFACE -Wall -Wextra -Wpedantic)

add_executable(gridflow_server src/main.cpp)
target_link_libraries(gridflow_server PRIVATE gridflow_core Threads::Threads)

enable_testing()
add_executable(gridflow_tests tests/test_main.cpp tests/test_smoke.cpp)
target_link_libraries(gridflow_tests PRIVATE gridflow_core)
add_test(NAME all_tests COMMAND gridflow_tests)