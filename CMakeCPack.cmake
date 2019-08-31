# This file is for cpack configuration only.
# It should only contain cpack-specific configuration instructions,
# e.g. no install or fixup_bundle code (which would be written to cmake_install.cmake, which we don't want).
# It also must be in the top project folder.
set (CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
include (InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "open_iA")
set(CPACK_PACKAGE_VENDOR "University of Applied Sciences Upper Austria, Campus Wels")
set(CPACK_PACKAGE_CONTACT "c.heinzl@fh-wels.at")
set(CPACK_PACKAGE_VERSION "${openiA_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A tool for the visual analysis and processing of volumetric datasets, with a focus on industrial computed tomography.")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}-${openiA_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${openiA_VERSION}")
set(CPACK_PACKAGE_CHECKSUM "SHA512")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${openiA_VERSION}-source")
#SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/logo.bmp")

set(CPACK_PACKAGE_EXECUTABLES "" "")

# NSIS-specific settings:
set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/gui/open_iA.ico")
set(CPACK_NSIS_MENU_LINKS "./${CPACK_PACKAGE_NAME}.exe" "${CPACK_PACKAGE_NAME} ${openiA_VERSION}")
set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME} ${openiA_VERSION}")

# DEB-specific settings:
# TODO: not sure yet how to handle dependencies properly
#SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Bernhard Froehler")
#MESSAGE(STATUS "${CPACK_DEBIAN_PACKAGE_DEPENDS}")
#SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.23)")

include(CPack)
