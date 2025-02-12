if(NOT CMAKE_CROSSCOMPILING AND NOT DISABLE_PKG AND EXISTS /etc/debian_version)
    set(DEB_DEPEND "")
    file(READ /etc/debian_version DEBIAN_VERSION)
    string(REGEX MATCH "^[0-9]+" DEBIAN_MAJOR "${DEBIAN_VERSION}")
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "python3")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_PACKAGE_VERSION_PATCH "${CMAKE_PROJECT_VERSION_PATCH}-deb${DEBIAN_MAJOR}")
    set(CPACK_PACKAGE_DESCRIPTION "globalsettings library")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Fabien Proriol <fabien.proriol@viavisolutions.com>")
    include(CPack)
endif()
