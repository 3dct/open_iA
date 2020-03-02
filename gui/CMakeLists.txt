# Generate executable:
SET (MAIN_SOURCES "${CMAKE_SOURCE_DIR}/gui/main.cpp")

IF (WIN32)
	set(MAIN_SOURCES ${MAIN_SOURCES} open_iA.rc)
	ADD_EXECUTABLE( ${GUI_EXECUTABLE_TARGET} WIN32 ${MAIN_SOURCES})
ELSE(WIN32)
	ADD_EXECUTABLE( ${GUI_EXECUTABLE_TARGET} ${MAIN_SOURCES} )
ENDIF (WIN32)

TARGET_LINK_LIBRARIES(${GUI_EXECUTABLE_TARGET} PRIVATE ${CORE_LIBRARY_NAME})
IF (WIN32)
	TARGET_LINK_LIBRARIES(${GUI_EXECUTABLE_TARGET} PRIVATE Qt5::WinMain)
ENDIF (WIN32)

TARGET_INCLUDE_DIRECTORIES(${GUI_EXECUTABLE_TARGET} PRIVATE ${open_iA_BINARY_DIR}) # for version.h

# Windows-specific configuration
IF (MSVC)
	IF (MSVC_VERSION GREATER_EQUAL 1910)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
	ENDIF()

	IF (NOT EXISTS ${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj.user
		OR NOT EXISTS ${CMAKE_BINARY_DIR}/x64/open_iA_gui_Release.bat
		OR NOT "${WinDLLPaths}" STREQUAL "${CACHED_OPENIA_GUI_ENV_PATH}"
	)
		MESSAGE(STATUS "GUI LIBRARY paths changed new=${WinDLLPaths}, cached=${CACHED_OPENIA_GUI_ENV_PATH}")
		# force cmake to copy the files:
		file(REMOVE ${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj.user)
		# create .user file FOR ALL_BUILD (for which we cannot use set_property / set_target_properties):
		CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/gui/ALL_BUILD.vcxproj.user ${CMAKE_BINARY_DIR}/ @ONLY)
		# make sure visual studio reloads the projects:
		execute_process(COMMAND cmake -E touch ${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj)
		SET (CACHED_OPENIA_GUI_ENV_PATH "${WinDLLPaths}" CACHE INTERNAL "" FORCE)
		set (EXE_NAME ${GUI_EXECUTABLE_FILE})
		foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
			string(REPLACE "$(Configuration)" "${CONFIG}" ADDITIONAL_PATH "${${CONFIG}LibraryPaths}")
			file(REMOVE ${CMAKE_BINARY_DIR}/x64/open_iA_gui_${CONFIG}.bat)
			CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmake/run-tmpl.bat ${CMAKE_BINARY_DIR}/x64/open_iA_gui_${CONFIG}.bat @ONLY)
		endforeach()
	ENDIF ()
	set_target_properties(${GUI_EXECUTABLE_TARGET} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${WinDLLPaths};%PATH%\nSCIFIO_PATH=${SCIFIO_PATH}")
ENDIF (MSVC)

IF (openiA_TESTING_ENABLED)
	IF (MSVC)
		# use the created .bat which makes sure all required dll's are in the path:
		SET (FileOpenTestBinary "${CMAKE_BINARY_DIR}/x64/open_iA_gui_Release.bat")
	ELSE()
		# when running under cron, it's not running in an X session - we need to emulate one with xvfb!
		SET (FileOpenTestBinary "${CMAKE_SOURCE_DIR}/test/run_with_xserver.sh" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${GUI_EXECUTABLE_TARGET}")
	ENDIF()
	ADD_TEST(NAME FileOpenTest COMMAND ${FileOpenTestBinary} ${CMAKE_SOURCE_DIR}/test/data/test2x2x2.mhd --quit 3000)
ENDIF()

IF (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE")
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
ENDIF()

# Installation
IF (FLATPAK_BUILD)
    INSTALL (TARGETS ${GUI_EXECUTABLE_TARGET} RUNTIME DESTINATION bin)
ELSE()
    INSTALL (TARGETS ${GUI_EXECUTABLE_TARGET} RUNTIME DESTINATION .)
ENDIF()
