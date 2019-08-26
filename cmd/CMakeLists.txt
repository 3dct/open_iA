SET (EXECUTABLE_NAME open_iA_cmd)

# Generate executable:
SET (MAIN_SOURCES "${open_iA_SOURCE_DIR}/cmd/main.cpp")
ADD_EXECUTABLE( ${EXECUTABLE_NAME} ${MAIN_SOURCES} )

TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} PRIVATE ${CORE_LIBRARY_NAME})
TARGET_INCLUDE_DIRECTORIES(${EXECUTABLE_NAME} PRIVATE ${CMAKE_BINARY_DIR}) # for version.h

IF (openiA_USE_COTIRE)
	cotire(${EXECUTABLE_NAME})
ENDIF()

# Windows-specific configuration
IF (MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
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
IF (BUILD_TESTING)
	IF (MSVC)
		# use the created .bat which makes sure all required dll's are in the path:
		SET (cmd_binary "${CMAKE_BINARY_DIR}/x64/open_iA_cmd_Release.bat")
	ELSE()
		SET (cmd_binary "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${EXECUTABLE_NAME}")
	ENDIF()
# Datatype conversion:
	ADD_TEST(NAME CMD_DatatypeConversion1 COMMAND ${cmd_binary} -r "Datatype Conversion" -i ${CMAKE_SOURCE_DIR}/test/data/test2x2x2.mhd -o ${CMAKE_BINARY_DIR}/test_datatypeconv1.mhd -p VTK_FLOAT true true 0 0 false 0 1 -q)
	ADD_TEST(NAME CMD_DatatypeConversion2 COMMAND ${cmd_binary} -r "Datatype Conversion" -i ${CMAKE_SOURCE_DIR}/test/data/test2x2x2.mhd -o ${CMAKE_BINARY_DIR}/test_datatypeconv2.mhd -p VTK_UNSIGNED_CHAR true true 0 0 true 0 0 -q)
# Intensity:
	ADD_TEST(NAME CMD_Invert COMMAND ${cmd_binary} -r "Invert" -i ${CMAKE_SOURCE_DIR}/test/data/test2x2x2.mhd -o ${CMAKE_BINARY_DIR}/test_invert.mhd -p true 1 -q)
	# TODO: check output?
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
