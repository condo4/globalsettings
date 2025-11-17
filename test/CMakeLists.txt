cmake_minimum_required(VERSION 3.15)
project(GlobalSettingsTests)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable strict compilation flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")

# Add thread support
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Find the library in parent build directory
set(GLOBALSETTINGS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build")
set(GLOBALSETTINGS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Check if library exists
if(NOT EXISTS "${GLOBALSETTINGS_LIB_DIR}/libglobalsettings.so")
    message(FATAL_ERROR
        "GlobalSettings library not found!\n"
        "Please build the library first:\n"
        "  cd ${CMAKE_CURRENT_SOURCE_DIR}/..\n"
        "  mkdir -p build && cd build\n"
        "  cmake .. && make\n")
endif()

# Include directories
include_directories(${GLOBALSETTINGS_INCLUDE_DIR})

# Add the library
add_library(globalsettings SHARED IMPORTED)
set_target_properties(globalsettings PROPERTIES
    IMPORTED_LOCATION "${GLOBALSETTINGS_LIB_DIR}/libglobalsettings.so"
)

# Executable for comprehensive test suite (test_all)
add_executable(test test_all.cpp)
target_link_libraries(test
    globalsettings
    Threads::Threads
)

# Individual test executables
add_executable(test_basic test_basic_functionality.cpp)
target_link_libraries(test_basic
    globalsettings
    Threads::Threads
)

add_executable(test_validation test_input_validation.cpp)
target_link_libraries(test_validation
    globalsettings
    Threads::Threads
)

add_executable(test_threads test_thread_safety.cpp)
target_link_libraries(test_threads
    globalsettings
    Threads::Threads
)

add_executable(test_path_traversal test_path_traversal.cpp)
target_link_libraries(test_path_traversal
    globalsettings
    Threads::Threads
)

add_executable(test_functional test_functional.cpp)
target_link_libraries(test_functional
    globalsettings
    Threads::Threads
)

# Set rpath so executables can find the library
set_target_properties(test PROPERTIES
    BUILD_RPATH "${GLOBALSETTINGS_LIB_DIR}"
    INSTALL_RPATH "${GLOBALSETTINGS_LIB_DIR}"
)

set_target_properties(test_basic PROPERTIES
    BUILD_RPATH "${GLOBALSETTINGS_LIB_DIR}"
    INSTALL_RPATH "${GLOBALSETTINGS_LIB_DIR}"
)

set_target_properties(test_validation PROPERTIES
    BUILD_RPATH "${GLOBALSETTINGS_LIB_DIR}"
    INSTALL_RPATH "${GLOBALSETTINGS_LIB_DIR}"
)

set_target_properties(test_threads PROPERTIES
    BUILD_RPATH "${GLOBALSETTINGS_LIB_DIR}"
    INSTALL_RPATH "${GLOBALSETTINGS_LIB_DIR}"
)

set_target_properties(test_path_traversal PROPERTIES
    BUILD_RPATH "${GLOBALSETTINGS_LIB_DIR}"
    INSTALL_RPATH "${GLOBALSETTINGS_LIB_DIR}"
)

set_target_properties(test_functional PROPERTIES
    BUILD_RPATH "${GLOBALSETTINGS_LIB_DIR}"
    INSTALL_RPATH "${GLOBALSETTINGS_LIB_DIR}"
)

# Print instructions
message(STATUS "")
message(STATUS "═══════════════════════════════════════════════════════")
message(STATUS "  GlobalSettings Test Suite Configuration")
message(STATUS "═══════════════════════════════════════════════════════")
message(STATUS "")
message(STATUS "Library found: ${GLOBALSETTINGS_LIB_DIR}/libglobalsettings.so")
message(STATUS "")
message(STATUS "Available test executables:")
message(STATUS "  ./test              - Run ALL tests (comprehensive suite)")
message(STATUS "  ./test_basic        - Run basic functionality tests")
message(STATUS "  ./test_validation   - Run input validation tests")
message(STATUS "  ./test_threads      - Run thread safety tests")
message(STATUS "  ./test_path_traversal - Run path traversal protection tests")
message(STATUS "  ./test_functional   - Run functional tests (needs namespace)")
message(STATUS "")
message(STATUS "Shell scripts:")
message(STATUS "  ../run_with_namespace.sh - Run functional tests in namespace")
message(STATUS "  ./test_cli_crash.sh - Run CLI crash protection tests")
message(STATUS "  ./test_python_argv.py - Run Python argv injection tests")
message(STATUS "")
message(STATUS "To run all tests:")
message(STATUS "  make && ./test")
message(STATUS "")
message(STATUS "═══════════════════════════════════════════════════════")
