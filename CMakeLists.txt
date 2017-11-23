CMAKE_MINIMUM_REQUIRED ( VERSION 3.1 )

PROJECT ( open_iA LANGUAGES CXX )

CMAKE_POLICY(SET CMP0053 OLD)	# suppress warnings from Qt config
CMAKE_POLICY(SET CMP0054 NEW)	# suppress warning about variable expansion

SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")
SET (Toolkit_DIR "${PROJECT_SOURCE_DIR}/Toolkit")
INCLUDE(Common)

IF (CMAKE_COMPILER_IS_GNUCXX)
	IF ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
		SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3")
		SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb3")
	ENDIF()
ENDIF()

#-------------------------
# Subdirectories
#-------------------------
ADD_SUBDIRECTORY(core)
ADD_SUBDIRECTORY(modules)
ADD_SUBDIRECTORY(Toolkit)
ADD_SUBDIRECTORY(gui)
ADD_SUBDIRECTORY(cmd)

#-------------------------
# Testing
#-------------------------
# Configure the default TESTING_DATA_ROOT for the location of Data.
set(TESTING_DATA_OUTPUT ${PROJECT_SOURCE_DIR}/Test_files/TestITK/Output)
#ITK testing
option (open_iA_TEST_ITK "" OFF)
if(open_iA_TEST_ITK)
	add_subdirectory("${PROJECT_SOURCE_DIR}/Test_files/TestITK")
endif(open_iA_TEST_ITK)

#-------------------------
# Installation
#-------------------------
# Text files:
INSTALL(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION .)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/README.md DESTINATION .)
if (UNIX)
	install(CODE "include(\"${CMAKE_SOURCE_DIR}/cmake/ChangeRPath.cmake\")")
endif()
include (CMakeCPack.cmake)
