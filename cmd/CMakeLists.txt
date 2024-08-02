set(EXECUTABLE_NAME open_iA_cmd)

# Generate executable:
set(MAIN_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/iACommandLineProcessor.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/iACommandLineProcessor.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/iALoggerStdOut.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/iALoggerStdOut.h")
add_executable( ${EXECUTABLE_NAME} ${MAIN_SOURCES} )

target_link_libraries(${EXECUTABLE_NAME} PRIVATE iA::guibase)
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${CMAKE_BINARY_DIR}) # for version.h

# Windows-specific configuration
if (MSVC)
	if (NOT "${WinDLLPaths}" STREQUAL "${CACHED_OPENIA_CMD_ENV_PATH}"
		OR NOT EXISTS ${CMAKE_BINARY_DIR}/x64/open_iA_cmd_Release.bat
	)
		message(STATUS "CMD LIBRARY paths changed new=${WinDLLPaths}, cached=${CACHED_OPENIA_CMD_ENV_PATH}")
		set(CACHED_OPENIA_CMD_ENV_PATH "${WinDLLPaths}" CACHE INTERNAL "" FORCE)
		set(EXE_NAME "${EXECUTABLE_NAME}${CMAKE_EXECUTABLE_SUFFIX}")
		foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
			string(REPLACE "$(Configuration)" "${CONFIG}" ADDITIONAL_PATH "${WinDLLPaths}")
			file(REMOVE ${CMAKE_BINARY_DIR}/x64/open_iA_cmd_${CONFIG}.bat)
			CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/run-tmpl.bat ${CMAKE_BINARY_DIR}/x64/open_iA_cmd_${CONFIG}.bat @ONLY)
		endforeach()
	endif()
	set_target_properties(${EXECUTABLE_NAME} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${WinDLLPaths};%PATH%\nSCIFIO_PATH=${SCIFIO_PATH}")
	target_sources(${EXECUTABLE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/WindowsApplicationUseUtf8.manifest)
endif()

# set the path to the command line test runner application:
if (openiA_TESTING_ENABLED)
	if (MSVC)
		# use the created .bat which makes sure all required dll's are in the path:
		set(TEST_CMD_EXECUTABLE "${CMAKE_BINARY_DIR}/x64/open_iA_cmd_Release.bat" PARENT_SCOPE)
	elseif(APPLE)
		set(TEST_CMD_EXECUTABLE "${CMAKE_BINARY_DIR}/x64/open_iA_cmd" PARENT_SCOPE)
	else()
		set(TEST_CMD_EXECUTABLE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXECUTABLE_NAME}" PARENT_SCOPE)
	endif()
endif()

if (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	target_compile_options(${EXECUTABLE_NAME} PRIVATE "-fPIE")
endif()

# Installation
if (FLATPAK_BUILD)
	install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION bin)
elseif (APPLE)
	install(TARGETS ${EXECUTABLE_NAME} BUNDLE DESTINATION .)
else()
	install(TARGETS ${EXECUTABLE_NAME} RUNTIME DESTINATION .)
endif()
