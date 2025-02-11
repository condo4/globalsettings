cmake_minimum_required(VERSION 3.14)

project(globalsettings VERSION 1.0.1 LANGUAGES CXX)

option(WITH_PYTHON "Create python binding" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(globalsettings SHARED
  src/globalsettings.h src/globalsettings.cpp
  src/changewatcher.h src/changewatcher.cpp
)
set_target_properties(globalsettings PROPERTIES VERSION ${CMAKE_PROJECT_VERSION} SOVERSION 1)
set_target_properties(globalsettings PROPERTIES PUBLIC_HEADER "src/globalsettings.h")

add_executable(settings src/settings.cpp)
target_include_directories(settings PRIVATE src)
target_link_libraries(settings globalsettings)

install(TARGETS globalsettings
    EXPORT "GlobalSettingsTargets"
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_PREFIX}/include/globalsettings"
)

install(TARGETS settings
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WITH_PYTHON)
    message(STATUS "Build with python binding")
    find_package(PythonLibs REQUIRED)
    set(PYTHON_MODULE_EXTENSION ".so" CACHE INTERNAL "Cross python lib extension")
    find_package(pybind11 REQUIRED)
    find_package(Python3 COMPONENTS Development REQUIRED)
    include_directories(${Python3_INCLUDE_DIRS})

    if ("${PYTHONPATH}" STREQUAL "")
        string(REGEX MATCH "libpython(.*).so" PYTHON_LIBRARY_FILENAME ${PYTHON_LIBRARY})
        string(REGEX REPLACE "(lib)(python.*)(.so)" "\\2" PYCODE ${PYTHON_LIBRARY_FILENAME})
        set(PYTHONPATH /usr/lib/${PYCODE}/)
    endif()

    pybind11_add_module(GlobalSettings python/bindings.cpp)
    target_link_libraries(GlobalSettings PUBLIC globalsettings ${PYTHON_LIBRARY})
    install(TARGETS GlobalSettings LIBRARY DESTINATION ${PYTHONPATH})
endif(WITH_PYTHON)


# CMake Module
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/generated/GlobalSettingsConfigVersion.cmake" COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "cmake/Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/GlobalSettingsConfig.cmake"
    INSTALL_DESTINATION "lib/cmake/${PROJECT_NAME}"
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/generated/GlobalSettingsConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/generated/GlobalSettingsConfigVersion.cmake"
    DESTINATION "lib/cmake/${PROJECT_NAME}"
)

install(
    EXPORT "GlobalSettingsTargets"
    DESTINATION "lib/cmake/${PROJECT_NAME}"
)

include(cmake/CPack.cmake)
