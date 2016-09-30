CMAKE_MINIMUM_REQUIRED ( VERSION 3.0 )

PROJECT ( open_iA_gui LANGUAGES CXX )

SET (EXECUTABLE_NAME open_iA)

# open_iA Version number
include(GetGitRevisionDescription)
git_describe(VERSION --tags)
configure_file("${open_iA_SOURCE_DIR}/cmake/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

# Generate executable:
SET (MAIN_SOURCES "${PROJECT_SOURCE_DIR}/main.cpp")

IF (WIN32)
	set(MAIN_SOURCES ${MAIN_SOURCES} open_iA.rc)
	ADD_EXECUTABLE( ${EXECUTABLE_NAME} WIN32 ${MAIN_SOURCES})
ELSE(WIN32)
	ADD_EXECUTABLE( ${EXECUTABLE_NAME} ${MAIN_SOURCES} )
ENDIF (WIN32)

TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} ${CORE_LIBRARY_NAME})
TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} ${QT_LIBRARIES})
IF (WIN32)
	TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} Qt5::WinMain)
	IF (VTK_VERSION_MAJOR GREATER 6)
		TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} Opengl32)
	ENDIF()
ENDIF (WIN32)
TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} ${VTK_LIBRARIES})
IF (CMAKE_COMPILER_IS_GNUCXX AND VTK_VERSION_MAJOR GREATER 6)
	TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} "GL")
ENDIF ()
TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} ${VTKEdge_LIBRARIES})
TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} ${ITK_LIBRARIES})
TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} ${Toolkit_LIBRARIES})

IF(open_iA_GPU_USING_OPENCL)
	TARGET_LINK_LIBRARIES(${EXECUTABLE_NAME} ${OPENCL_LIBRARIES}) #TODO: proper OpenCL connection
ENDIF(open_iA_GPU_USING_OPENCL)

INCLUDE_DIRECTORIES( ${open_iA_core_SOURCE_DIR}/src )
INCLUDE_DIRECTORIES( ${open_iA_core_BINARY_DIR} ) # for open_iACore_export.h
INCLUDE_DIRECTORIES( ${open_iA_gui_BINARY_DIR} ) # for version.h

# Windows-specific configuration
IF (WIN32)
	ADD_DEFINITIONS(-DCL_COMP)
	IF (MSVC)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /EHsc")
		# Activate use of multi-processor compilation if we have Microsft Visual C++ compiler.
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

		ADD_DEFINITIONS(/D _CRT_SECURE_NO_WARNINGS)
		ADD_DEFINITIONS(/D _SCL_SECURE_NO_WARNINGS)

		# Apply file grouping based on regular expressions for Visual Studio IDE.
		SOURCE_GROUP("UI Files" REGULAR_EXPRESSION "[.](ui|qrc)$")
		
		# Set up debugging/running environments	in Visual Studio to point to the correct dll files:
		STRING(REGEX REPLACE "/" "\\\\" VTK_WIN_DIR ${VTK_DIR})
		STRING(REGEX REPLACE "/" "\\\\" ITK_WIN_DIR ${ITK_DIR})
		STRING(REGEX REPLACE "/lib/cmake/Qt5" "" Qt5_WIN_DIR ${Qt5_DIR})
		STRING(REGEX REPLACE "/" "\\\\" Qt5_WIN_DIR ${Qt5_WIN_DIR})
		#MESSAGE(STATUS "VTK: ${VTK_WIN_DIR} ITK: ${ITK_WIN_DIR} Qt5: ${Qt5_WIN_DIR}")
		SET (RelWithDebInfoLibraryPaths "${VTK_WIN_DIR}\\bin\\RelWithDebInfo;${ITK_WIN_DIR}\\bin\\RelWithDebInfo;${Qt5_WIN_DIR}\\bin")
		SET (ReleaseLibraryPaths "${VTK_WIN_DIR}\\bin\\Release;${ITK_WIN_DIR}\\bin\\Release;${Qt5_WIN_DIR}\\bin")
		SET (DebugLibraryPaths "${VTK_WIN_DIR}\\bin\\Debug;${ITK_WIN_DIR}\\bin\\Debug;${Qt5_WIN_DIR}\\bin")
		SET (MinSizeRelLibraryPaths "${VTK_WIN_DIR}\\bin\\MinSizeRel;${ITK_WIN_DIR}\\bin\\MinSizeRel;${Qt5_WIN_DIR}\\bin")
		CONFIGURE_FILE(${open_iA_gui_SOURCE_DIR}/open_iA.vcxproj.user ${open_iA_gui_BINARY_DIR} @ONLY)

	ENDIF (MSVC)
ENDIF (WIN32)


# Installation
INSTALL (TARGETS ${EXECUTABLE_NAME} DESTINATION .)
