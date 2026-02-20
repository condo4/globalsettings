cmake_minimum_required(VERSION 3.14)

project(globalsettings VERSION 3.0.0 LANGUAGES CXX)

option(WITH_PYTHON "Create python binding" ON)
option(WITH_QT     "Create Qt binding"     ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

set(CMakeModuleName "GlobalSettings")
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(version_config "${generated_dir}/${CMakeModuleName}ConfigVersion.cmake")
set(project_config "${generated_dir}/${CMakeModuleName}Config.cmake")
set(targets_export_name "${CMakeModuleName}Targets")
set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")


# Enable strict compilation flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")

find_package (Threads REQUIRED)

add_library(globalsettings SHARED
  src/globalsettings.h src/globalsettings.cpp
  src/changewatcher.h src/changewatcher.cpp
  src/utilssettings.h
)

target_include_directories(globalsettings
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include/globalsettings>
    )


if(WITH_QT)
  set(CMakeModuleQtName "GlobalSettingsQt")
  set(version_qt_config "${generated_dir}/${CMakeModuleQtName}ConfigVersion.cmake")
  set(project_qt_config "${generated_dir}/${CMakeModuleQtName}Config.cmake")
  set(targets_qt_export_name "${CMakeModuleQtName}Targets")
  set(config_qt_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}-qt")
  set(CMAKE_AUTOMOC ON)
  find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Qml REQUIRED)
  find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Qml REQUIRED)
  message(STATUS "Create bingings for Qt${QT_VERSION_MAJOR}")
  add_library(globalsettings-qt SHARED
    qt/globalsettingsprovider.h qt/globalsettingsprovider.cpp
    qt/globalproperty.h qt/globalproperty.cpp
  )
  target_include_directories(globalsettings-qt
    PRIVATE
      src
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
      $<INSTALL_INTERFACE:include/globalsettings>
  )
  target_link_libraries(globalsettings-qt globalsettings Qt${QT_VERSION_MAJOR}::Core)
  set_target_properties(globalsettings-qt PROPERTIES VERSION ${CMAKE_PROJECT_VERSION} SOVERSION 1)
  set_target_properties(globalsettings-qt PROPERTIES PUBLIC_HEADER "qt/globalproperty.h;qt/globalsettingsprovider.h")

  add_library(GlobalSettingsPlugin  SHARED qt/globalsettings_plugin.cpp qt/globalsettings_plugin.h)
  if(NOT DEFINED QML_MODULE_INSTALL_PATH)
    if(DEFINED OE_QMAKE_PATH_QML)
      set(QML_MODULE_INSTALL_PATH ${OE_QMAKE_PATH_QML} )
    elseif(BUILD_DEBIAN_PACKAGE)
      set(QML_MODULE_INSTALL_PATH "/usr/lib/x86_64-linux-gnu/qt${QT_VERSION_MAJOR}/qml")
    else()
      set(QML_MODULE_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/qml )
    endif()
  endif()
  target_link_libraries(GlobalSettingsPlugin
    PRIVATE
      Qt${QT_VERSION_MAJOR}::Qml
      globalsettings-qt
  )

  install(TARGETS GlobalSettingsPlugin  DESTINATION ${QML_MODULE_INSTALL_PATH}/GlobalSettings)
  install(FILES   qt/qml/qmldir         DESTINATION ${QML_MODULE_INSTALL_PATH}/GlobalSettings)
endif(WITH_QT)

if(DYNAMIC_OVERRIDE)
    target_compile_definitions(globalsettings PUBLIC -DDYNAMIC_OVERRIDE=${DYNAMIC_OVERRIDE})
endif(DYNAMIC_OVERRIDE)

set_target_properties(globalsettings PROPERTIES VERSION ${CMAKE_PROJECT_VERSION} SOVERSION 1)
set_target_properties(globalsettings PROPERTIES PUBLIC_HEADER "src/globalsettings.h;src/utilssettings.h")
target_link_libraries(globalsettings PRIVATE Threads::Threads)


add_executable(settings src/settings.cpp)
target_include_directories(settings PRIVATE src)
target_link_libraries(settings globalsettings)

install(TARGETS globalsettings
    EXPORT "${targets_export_name}"
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION "include/globalsettings"
)


install(TARGETS settings
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
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}"
)

install(
    FILES FILES "${project_config}" "${version_config}"
    DESTINATION "${config_install_dir}"
)

install(
    EXPORT "${targets_export_name}"
    NAMESPACE globalsettings::
    DESTINATION "${config_install_dir}"
)


if(WITH_QT)
  install(TARGETS globalsettings-qt
      EXPORT "GlobalSettingsQtTargets"
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_PREFIX}/include/globalsettings-qt"
  )
  write_basic_package_version_file(
      "${CMAKE_CURRENT_BINARY_DIR}/generated/GlobalSettingsQtConfigVersion.cmake" COMPATIBILITY SameMajorVersion
  )

  configure_package_config_file(
      "qt/cmake/Config.cmake.in"
      "${project_qt_config}"
      INSTALL_DESTINATION "${config_qt_install_dir}"
  )

  install(
      FILES FILES "${project_qt_config}" "${version_qt_config}"
      DESTINATION "${config_qt_install_dir}"
  )

  install(
      EXPORT "${targets_qt_export_name}"
      NAMESPACE globalsettings::
      DESTINATION "${config_qt_install_dir}"
  )
endif(WITH_QT)

include(cmake/CPack.cmake)
