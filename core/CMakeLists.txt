CMAKE_MINIMUM_REQUIRED ( VERSION 3.0 )

PROJECT ( open_iA_core LANGUAGES CXX )

#Collect .cpp and .h sources from the sources directory
FILE( GLOB open_iA_core_SRCS "src/*.cpp" "src/*.h" )

# *.ui files
FILE( GLOB open_iA_core_UI "src/*.ui" )

# *.qrc files
SET( open_iA_core_QRC open_iA.qrc)


#========================================================================================-
# Finalize configuration
#========================================================================================-

# SET(CMAKE_AUTOUIC ON)
# build ui_XXX files from the XML-style .ui files
QT5_WRAP_UI( open_iA_core_SRCS ${open_iA_core_UI})
QT5_ADD_RESOURCES( open_iA_core_SRCS ${open_iA_core_QRC} )

SET (LIB_TYPE SHARED)

# Generate core dll file:
add_library(${CORE_LIBRARY_NAME} ${LIB_TYPE} ${open_iA_core_SRCS} ${open_iA_core_QRC} )
TARGET_LINK_LIBRARIES(${CORE_LIBRARY_NAME} ${QT_LIBRARIES})
TARGET_LINK_LIBRARIES(${CORE_LIBRARY_NAME} ${VTK_LIBRARIES})
TARGET_LINK_LIBRARIES(${CORE_LIBRARY_NAME} ${ITK_LIBRARIES})
IF (WIN32)
	# apparently required for VS 2015, and doesn't hurt for VS2013:
	TARGET_LINK_LIBRARIES(${CORE_LIBRARY_NAME} Opengl32)
ENDIF (WIN32)

INCLUDE(${open_iA_SOURCE_DIR}/cmake/Modules/Modules.cmake)
MODULE_GENERATE_EXPORT_HEADER ( ${CORE_LIBRARY_NAME} )

INCLUDE_DIRECTORIES( ${open_iA_core_SOURCE_DIR}/src )
INCLUDE_DIRECTORIES( ${open_iA_core_BINARY_DIR} )

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
	ENDIF (MSVC)
ENDIF (WIN32)

# Installation
INSTALL (TARGETS ${CORE_LIBRARY_NAME} DESTINATION bin)