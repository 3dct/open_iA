SET (EXECUTABLE_NAME open_iA_cmd)

# open_iA Version number
include(GetGitRevisionDescription)
git_describe(VERSION --tags)
configure_file("${open_iA_SOURCE_DIR}/cmake/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

# Generate executable:
SET (MAIN_SOURCES "${open_iA_SOURCE_DIR}/cmd/main.cpp")
ADD_EXECUTABLE( ${EXECUTABLE_NAME} ${MAIN_SOURCES} )

TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} PRIVATE ${CORE_LIBRARY_NAME})
TARGET_INCLUDE_DIRECTORIES(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}) # for version.h

IF (USE_COTIRE)
	cotire(${EXECUTABLE_NAME})
ENDIF()

# Windows-specific configuration
IF (MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /permissive-")

	IF (NOT "${DebugLibraryPaths}" STREQUAL "${CACHED_OPENIA_CMD_DBG_ENV_PATH}")
		# force cmake to copy the files:
		file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/open_iA_cmd.vcxproj.user)
		# create .user files:
		CONFIGURE_FILE(${open_iA_SOURCE_DIR}/cmd/open_iA_cmd.vcxproj.user ${CMAKE_CURRENT_BINARY_DIR}/ @ONLY)
		# make sure visual studio reloads the projects:
		execute_process(COMMAND cmake -E touch ${CMAKE_CURRENT_BINARY_DIR}/open_iA_cmd.vcxproj)
		SET (CACHED_OPENIA_CMD_DBG_ENV_PATH "${DebugLibraryPaths}" CACHE INTERNAL "" FORCE)

		SET (ExeName "open_iA_cmd.exe")
		foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
			set (ADDITIONAL_PATH ${${CONFIG}LibraryPaths})
			file(REMOVE ${CMAKE_BINARY_DIR}/x64/open_iA_cmd_${CONFIG}.bat )
			CONFIGURE_FILE(${open_iA_SOURCE_DIR}/cmake/run-tmpl.bat ${CMAKE_BINARY_DIR}/x64/open_iA_cmd_${CONFIG}.bat @ONLY)
		endforeach()
	ENDIF ()
ENDIF (MSVC)

IF (CMAKE_COMPILER_IS_GNUCXX)
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE")
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
ENDIF()

# Installation
INSTALL (TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION .)
