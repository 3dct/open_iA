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

# Windows-specific configuration
IF (MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /permissive-")

	IF (NOT "${DebugLibraryPaths}" STREQUAL "${CACHED_OPENIA_CMD_DBG_ENV_PATH}")
		MESSAGE(STATUS "CMD: Updating PATH in .user files")
		MESSAGE(STATUS "  cached=${CACHED_OPENIA_CMD_DBG_ENV_PATH}")
		MESSAGE(STATUS "     new=${DebugLibraryPaths}")
		# force cmake to copy the files:
		file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/open_iA_cmd.vcxproj.user)
		# create .user files:
		CONFIGURE_FILE(${open_iA_SOURCE_DIR}/cmd/open_iA_cmd.vcxproj.user ${CMAKE_CURRENT_BINARY_DIR}/ @ONLY)
		# make sure visual studio reloads the projects:
		execute_process(COMMAND cmake -E touch ${CMAKE_CURRENT_BINARY_DIR}/open_iA_cmd.vcxproj.user)
		SET (CACHED_OPENIA_CMD_DBG_ENV_PATH "${DebugLibraryPaths}" CACHE INTERNAL "" FORCE)
	ENDIF ()
ENDIF (MSVC)

IF (CMAKE_COMPILER_IS_GNUCXX)
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE")
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
ENDIF()

# Installation
INSTALL (TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION .)
