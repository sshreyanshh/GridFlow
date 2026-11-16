cmake_minimum_required(VERSION 3.16)
project(gridflow LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
find_package(Threads REQUIRED)

# Wave 1–5 source and test files are registered by these CONFIGURE_DEPENDS globs.
# Every .cpp under src/ except the two entry points is library code shared by apps and tests.
file(GLOB_RECURSE GF_LIB_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(FILTER GF_LIB_SOURCES EXCLUDE REGEX "/(main|terminal_main)\\.cpp$")

# Every .cpp under tests/ is a test file.
file(GLOB_RECURSE GF_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)

add_library(gridflow_settings INTERFACE)
target_include_directories(gridflow_settings INTERFACE include src third_party)
target_compile_options(gridflow_settings INTERFACE -Wall -Wextra -Wpedantic)

add_executable(gridflow_server src/main.cpp ${GF_LIB_SOURCES})
target_link_libraries(gridflow_server PRIVATE gridflow_settings Threads::Threads)

add_executable(gridflow_terminal src/terminal_main.cpp ${GF_LIB_SOURCES})
target_link_libraries(gridflow_terminal PRIVATE gridflow_settings Threads::Threads)

enable_testing()
add_executable(gridflow_tests ${GF_TEST_SOURCES} ${GF_LIB_SOURCES})
target_link_libraries(gridflow_tests PRIVATE gridflow_settings Threads::Threads)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.." GRIDFLOW_REPO_ROOT)
target_compile_definitions(gridflow_tests PRIVATE
    GRIDFLOW_REPO_ROOT="${GRIDFLOW_REPO_ROOT}"
    GRIDFLOW_DATA_DIR="${GRIDFLOW_REPO_ROOT}/data"
)
add_test(NAME all_tests COMMAND gridflow_tests)

option(GRIDFLOW_ENABLE_SANITIZERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
if(GRIDFLOW_ENABLE_SANITIZERS)
    if(MSVC)
        message(FATAL_ERROR "GRIDFLOW_ENABLE_SANITIZERS requires a compiler supporting -fsanitize=address,undefined")
    endif()

    target_compile_options(gridflow_settings INTERFACE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(gridflow_settings INTERFACE -fsanitize=address,undefined)
endif()
