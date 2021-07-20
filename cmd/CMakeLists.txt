SET (EXECUTABLE_NAME open_iA_cmd)

# Generate executable:
SET (MAIN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/iACommandLineProcessor.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/iACommandLineProcessor.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/iALoggerStdOut.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/iALoggerStdOut.h")
ADD_EXECUTABLE( ${EXECUTABLE_NAME} ${MAIN_SOURCES} )

TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} PRIVATE iAguibase)
TARGET_INCLUDE_DIRECTORIES(${EXECUTABLE_NAME} PRIVATE ${CMAKE_BINARY_DIR}) # for version.h

# Windows-specific configuration
IF (MSVC)
	IF (NOT "${WinDLLPaths}" STREQUAL "${CACHED_OPENIA_CMD_ENV_PATH}"
		OR NOT EXISTS ${CMAKE_BINARY_DIR}/x64/open_iA_cmd_Release.bat
	)
		MESSAGE(STATUS "CMD LIBRARY paths changed new=${WinDLLPaths}, cached=${CACHED_OPENIA_CMD_ENV_PATH}")
		SET (CACHED_OPENIA_CMD_ENV_PATH "${WinDLLPaths}" CACHE INTERNAL "" FORCE)
		SET (EXE_NAME "${EXECUTABLE_NAME}${CMAKE_EXECUTABLE_SUFFIX}")
		foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
			string(REPLACE "$(Configuration)" "${CONFIG}" ADDITIONAL_PATH "${WinDLLPaths}")
			file(REMOVE ${CMAKE_BINARY_DIR}/x64/open_iA_cmd_${CONFIG}.bat)
			CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/run-tmpl.bat ${CMAKE_BINARY_DIR}/x64/open_iA_cmd_${CONFIG}.bat @ONLY)
		endforeach()
	ENDIF ()
	set_target_properties(${EXECUTABLE_NAME} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${WinDLLPaths};%PATH%\nSCIFIO_PATH=${SCIFIO_PATH}")
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

IF (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	target_compile_options(${EXECUTABLE_NAME} PRIVATE "-fPIE")
ENDIF()

# Installation
IF (FLATPAK_BUILD)
	INSTALL (TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)
ELSE()
	INSTALL (TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION .)
ENDIF()
