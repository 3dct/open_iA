CMAKE_MINIMUM_REQUIRED ( VERSION 3.14 )
# 3.13 required for VS_DEBUGGER_ENVIRONMENT!
# 3.14 required for CMAKE_POLICY CMP0087 / Generator expressions in install code|script

PROJECT ( open_iA LANGUAGES CXX )

SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")
SET(FLATPAK_APPID at.zfp.openia)

SET (Toolkit_DIR "${PROJECT_SOURCE_DIR}/Toolkit")

INCLUDE(Common)

#-------------------------
# Subdirectories
#-------------------------
SET (TEST_DATA_DIR "${CMAKE_SOURCE_DIR}/test/data")
ADD_SUBDIRECTORY(core)

ADD_SUBDIRECTORY(cmd)     # Sets CMD test runner, used in modules

SET (GUI_EXECUTABLE_TARGET open_iA)  # required in modules to set dependency on it
SET (GUI_EXECUTABLE_FILE "${GUI_EXECUTABLE_TARGET}${CMAKE_EXECUTABLE_SUFFIX}")  # required in modules to use as debugger command
ADD_SUBDIRECTORY(modules)

# modules might have added something to BUILD_INFO:
configure_file("${open_iA_SOURCE_DIR}/cmake/version.h.in" "${open_iA_BINARY_DIR}/version.h" @ONLY)

ADD_SUBDIRECTORY(gui)

ADD_SUBDIRECTORY(xinst)

#-------------------------
# Installation
#-------------------------
# Text files:
INSTALL(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION .)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/README.md DESTINATION .)
IF (FLATPAK_BUILD)
	INSTALL(FILES ${CMAKE_SOURCE_DIR}/${FLATPAK_APPID}.appdata.xml DESTINATION app/share/metainfo)
ENDIF()
include (CMakeCPack.cmake)
