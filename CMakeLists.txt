cmake_minimum_required(VERSION 3.26...3.29 FATAL_ERROR)
# 3.26 required for copy_directory_if_different
project(open_iA LANGUAGES CXX)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
set(FLATPAK_APPID at.zfp.openia)

set(Toolkit_DIR "${PROJECT_SOURCE_DIR}/Toolkit")

include(Common)

#-------------------------
# Subdirectories
#-------------------------
set(TEST_DATA_DIR "${CMAKE_SOURCE_DIR}/test/data")
add_subdirectory(libs)

add_subdirectory(cmd)     # Sets CMD test runner, used in modules

set(GUI_EXECUTABLE_TARGET open_iA)  # required in modules to set dependency on it
set(GUI_EXECUTABLE_FILE "${GUI_EXECUTABLE_TARGET}${CMAKE_EXECUTABLE_SUFFIX}")  # required in modules to use as debugger command
add_subdirectory(modules)

# modules might have added something to BUILD_INFO:
configure_file("${CMAKE_SOURCE_DIR}/cmake/version.h.in" "${CMAKE_BINARY_DIR}/version.h" @ONLY)

add_subdirectory(gui)

if (openiA_TESTING_ENABLED)
	add_subdirectory(test)
endif()

#-------------------------
# Installation
#-------------------------
add_subdirectory(xinst)
# Text files:
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION .)
install(FILES ${CMAKE_SOURCE_DIR}/README.md DESTINATION .)
if (FLATPAK_BUILD)
	install(FILES ${CMAKE_SOURCE_DIR}/${FLATPAK_APPID}.appdata.xml DESTINATION share/metainfo)
endif()
include(CMakeCPack.cmake)
