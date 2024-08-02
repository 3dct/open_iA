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

add_subdirectory(cmd)     # Sets CMD test runner (TEST_CMD_EXECUTABLE), used in modules

set(GUI_EXECUTABLE_TARGET open_iA)  # required in modules to set dependency on it
set(GUI_EXECUTABLE_FILE "${GUI_EXECUTABLE_TARGET}${CMAKE_EXECUTABLE_SUFFIX}")  # required in modules to use as debugger command
if (openiA_TESTING_ENABLED)
	if (MSVC)
		# use the created .bat which makes sure all required dll's are in the path:
		set(TEST_GUI_EXECUTABLE "${CMAKE_BINARY_DIR}/x64/open_iA_gui_Release.bat")
	elseif(APPLE)
		set(TEST_GUI_EXECUTABLE "${CMAKE_BINARY_DIR}/x64/Release/open_iA.app/Contents/MacOS/open_iA")
	else()
		# make sure the Mesa OpenGL Override version is set
		set(TEST_GUI_EXECUTABLE "${CMAKE_SOURCE_DIR}/test/run_with_gl.sh" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${GUI_EXECUTABLE_TARGET}")
	endif()

endif()

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
