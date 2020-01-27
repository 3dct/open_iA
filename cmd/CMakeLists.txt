SET (EXECUTABLE_NAME open_iA_cmd)

# Generate executable:
SET (MAIN_SOURCES "${open_iA_SOURCE_DIR}/cmd/main.cpp")
ADD_EXECUTABLE( ${EXECUTABLE_NAME} ${MAIN_SOURCES} )

TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} PRIVATE ${CORE_LIBRARY_NAME})
TARGET_INCLUDE_DIRECTORIES(${EXECUTABLE_NAME} PRIVATE ${open_iA_BINARY_DIR}) # for version.h

# Windows-specific configuration
IF (MSVC)
	IF (MSVC_VERSION GREATER_EQUAL 1910)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
	ENDIF()

	IF (NOT "${DebugLibraryPaths}" STREQUAL "${CACHED_OPENIA_CMD_DBG_ENV_PATH}")
		# force cmake to copy the files:
		file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/open_iA_cmd.vcxproj.user)
		# create .user files:
		CONFIGURE_FILE(${open_iA_SOURCE_DIR}/cmd/open_iA_cmd.vcxproj.user ${CMAKE_CURRENT_BINARY_DIR}/ @ONLY)
		# make sure visual studio reloads the projects:
		execute_process(COMMAND cmake -E touch ${CMAKE_CURRENT_BINARY_DIR}/open_iA_cmd.vcxproj)
		SET (CACHED_OPENIA_CMD_DBG_ENV_PATH "${DebugLibraryPaths}" CACHE INTERNAL "" FORCE)

		SET (EXE_NAME "${EXECUTABLE_NAME}${CMAKE_EXECUTABLE_SUFFIX}")
		foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
			set (ADDITIONAL_PATH ${${CONFIG}LibraryPaths})
			file(REMOVE ${CMAKE_BINARY_DIR}/x64/open_iA_cmd_${CONFIG}.bat )
			CONFIGURE_FILE(${open_iA_SOURCE_DIR}/cmake/run-tmpl.bat ${CMAKE_BINARY_DIR}/x64/open_iA_cmd_${CONFIG}.bat @ONLY)
		endforeach()
	ENDIF ()
ENDIF (MSVC)

# Test running filters from command line
# TODO: check output (at the moment, it only whether the filters run without crashing)!
IF (openiA_TESTING_ENABLED)
	IF (MSVC)
		# use the created .bat which makes sure all required dll's are in the path:
		SET (TEST_CMD_Binary "${CMAKE_BINARY_DIR}/x64/open_iA_cmd_Release.bat" PARENT_SCOPE)
	ELSE()
		SET (TEST_CMD_Binary "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXECUTABLE_NAME}" PARENT_SCOPE)
	ENDIF()
ENDIF()

IF (CMAKE_COMPILER_IS_GNUCXX)
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE")
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
ENDIF()

# Installation
IF (FLATPAK_BUILD)
	INSTALL (TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)
ELSE()
	INSTALL (TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION .)
ENDIF()
