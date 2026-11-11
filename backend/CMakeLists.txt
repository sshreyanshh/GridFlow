cmake_minimum_required(VERSION 3.16)
project(gridflow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Threads REQUIRED)

# Every .cpp under src/ except main.cpp is "library" code shared by server and tests.
file(GLOB_RECURSE GF_LIB_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(FILTER GF_LIB_SOURCES EXCLUDE REGEX "/main\\.cpp$")

# Every .cpp under tests/ is a test file.
file(GLOB_RECURSE GF_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)

add_library(gridflow_settings INTERFACE)
target_include_directories(gridflow_settings INTERFACE include third_party)
target_compile_options(gridflow_settings INTERFACE -Wall -Wextra -Wpedantic)

add_executable(gridflow_server src/main.cpp ${GF_LIB_SOURCES})
target_link_libraries(gridflow_server PRIVATE gridflow_settings Threads::Threads)

enable_testing()
add_executable(gridflow_tests ${GF_TEST_SOURCES} ${GF_LIB_SOURCES})
target_link_libraries(gridflow_tests PRIVATE gridflow_settings)
add_test(NAME all_tests COMMAND gridflow_tests)