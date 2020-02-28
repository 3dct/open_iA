CMAKE_MINIMUM_REQUIRED ( VERSION 3.13 )
# 3.13 required for VS_DEBUGGER_ENVIRONMENT!

PROJECT ( open_iA LANGUAGES CXX )

SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")

SET (Toolkit_DIR "${PROJECT_SOURCE_DIR}/Toolkit")

INCLUDE(Common)

#-------------------------
# Subdirectories
#-------------------------
SET (TEST_DATA_DIR "${CMAKE_SOURCE_DIR}/test/data")
ADD_SUBDIRECTORY(core)

ADD_SUBDIRECTORY(cmd)     # Sets CMD test runner, used in modules

SET (GUI_EXECUTABLE_TARGET open_iA)  # required already in modules to set dependency on it
ADD_SUBDIRECTORY(modules)

# modules might have added something to BUILD_INFO:
configure_file("${open_iA_SOURCE_DIR}/cmake/version.h.in" "${open_iA_BINARY_DIR}/version.h" @ONLY)

SET (GUI_EXECUTABLE_FILE "${GUI_EXECUTABLE_TARGET}${CMAKE_EXECUTABLE_SUFFIX}")
ADD_SUBDIRECTORY(gui)

ADD_SUBDIRECTORY(xinst)

#-------------------------
# Installation
#-------------------------
# Text files:
INSTALL(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION .)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/README.md DESTINATION .)
include (CMakeCPack.cmake)
