# Generate executable:
SET (MAIN_SOURCES "${CMAKE_SOURCE_DIR}/gui/main.cpp")

IF (WIN32)
	set(MAIN_SOURCES ${MAIN_SOURCES} open_iA.rc)
	ADD_EXECUTABLE( ${GUI_EXECUTABLE_NAME} WIN32 ${MAIN_SOURCES})
ELSE(WIN32)
	ADD_EXECUTABLE( ${GUI_EXECUTABLE_NAME} ${MAIN_SOURCES} )
ENDIF (WIN32)

TARGET_LINK_LIBRARIES(${GUI_EXECUTABLE_NAME} PRIVATE ${CORE_LIBRARY_NAME})
IF (WIN32)
	TARGET_LINK_LIBRARIES(${GUI_EXECUTABLE_NAME} PRIVATE Qt5::WinMain)
ENDIF (WIN32)

TARGET_INCLUDE_DIRECTORIES(${GUI_EXECUTABLE_NAME} PRIVATE ${open_iA_BINARY_DIR}) # for version.h

# Windows-specific configuration
IF (MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
	IF (MSVC_VERSION GREATER_EQUAL 1910)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
	ENDIF()

	IF (NOT EXISTS ${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj.user
		OR NOT EXISTS ${CMAKE_BINARY_DIR}/gui/open_iA.vcxproj.user
		OR NOT "${DebugLibraryPaths}" STREQUAL "${CACHED_OPENIA_GUI_DBG_ENV_PATH}")

		MESSAGE(STATUS "LIBRARY paths changed new=${DebugLibraryPaths}, cached=${CACHED_OPENIA_GUI_DBG_ENV_PATH}")
		# force cmake to copy the files:
		file(REMOVE ${CMAKE_BINARY_DIR}/gui/open_iA.vcxproj.user)
		file(REMOVE ${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj.user)
		# create .user files:
		CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/gui/open_iA.vcxproj.user ${CMAKE_BINARY_DIR}/gui @ONLY)
		CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/gui/ALL_BUILD.vcxproj.user ${CMAKE_BINARY_DIR}/ @ONLY)
		# make sure visual studio reloads the projects:
		execute_process(COMMAND cmake -E touch ${CMAKE_BINARY_DIR}/gui/open_iA.vcxproj)
		execute_process(COMMAND cmake -E touch ${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj)
		SET (CACHED_OPENIA_GUI_DBG_ENV_PATH "${DebugLibraryPaths}" CACHE INTERNAL "" FORCE)

		foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
			set (ADDITIONAL_PATH ${${CONFIG}LibraryPaths})
			file(REMOVE ${CMAKE_BINARY_DIR}/x64/open_iA_gui_${CONFIG}.bat )
			CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmake/run-tmpl.bat ${CMAKE_BINARY_DIR}/x64/open_iA_gui_${CONFIG}.bat @ONLY)
		endforeach()
	ENDIF ()
ENDIF (MSVC)

IF (openiA_TESTING_ENABLED)
	IF (MSVC)
		# use the created .bat which makes sure all required dll's are in the path:
		SET (FileOpenTestBinary "${CMAKE_BINARY_DIR}/x64/open_iA_gui_Release.bat")
	ELSE()
		# when running under cron, it's not running in an X session - we need to emulate one with xvfb!
		SET (FileOpenTestBinary "${CMAKE_SOURCE_DIR}/test/run_with_xserver.sh" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${GUI_EXECUTABLE_NAME}")
	ENDIF()
	ADD_TEST(NAME FileOpenTest COMMAND ${FileOpenTestBinary} ${CMAKE_SOURCE_DIR}/test/data/test2x2x2.mhd --quit 3000)
ENDIF()

IF (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE")
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
ENDIF()

# Installation
IF (FLATPAK_BUILD)
    INSTALL (TARGETS ${GUI_EXECUTABLE_NAME} RUNTIME DESTINATION bin)
ELSE()
    INSTALL (TARGETS ${GUI_EXECUTABLE_NAME} RUNTIME DESTINATION .)
ENDIF()
