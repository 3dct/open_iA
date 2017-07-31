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
TARGET_LINK_LIBRARIES(${CORE_LIBRARY_NAME} debug ${HDF5_hdf5_LIBRARY_DEBUG})
TARGET_LINK_LIBRARIES(${CORE_LIBRARY_NAME} optimized ${HDF5_hdf5_LIBRARY_RELEASE})
ADD_DEFINITIONS(-DH5_BUILT_AS_DYNAMIC_LIB)
IF (WIN32)
	# apparently required for VS 2015, and doesn't hurt for VS2013:
	TARGET_LINK_LIBRARIES(${CORE_LIBRARY_NAME} Opengl32)
ENDIF (WIN32)


IF(APPLE)
   INCLUDE_DIRECTORIES ( /System/Library/Frameworks )
   FIND_LIBRARY(COCOA_LIBRARY Cocoa)
   FIND_LIBRARY(GLUT_LIBRARY GLUT )
   FIND_LIBRARY(OpenGL_LIBRARY OpenGL )
   MARK_AS_ADVANCED (COCOA_LIBRARY
                     GLUT_LIBRARY
                     OpenGL_LIBRARY)
   SET(EXTRA_LIBS ${COCOA_LIBRARY} ${GLUT_LIBRARY} ${OpenGL_LIBRARY})
	 TARGET_LINK_LIBRARIES(${CORE_LIBRARY_NAME} ${EXTRA_LIBS})
	 MESSAGE(STATUS "Extra Core: ${EXTRA_LIBS}")
ENDIF (APPLE)


INCLUDE(Modules)
MODULE_GENERATE_EXPORT_HEADER ( ${CORE_LIBRARY_NAME} )

INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/core/src )
INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR}/core )

# Compiler Flags
IF (MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
ENDIF ()
IF (CMAKE_COMPILER_IS_GNUCXX)
	# see Ulrich Drepper - How to write Shared Libraries
	#   - fPIC vs. fpic
	#   - why fvisibility=hidden
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic -fvisibility=hidden")
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -fvisibility=hidden")
ENDIF()
IF( "${CMAKE_SYSTEM_NAME}" MATCHES "Darwin" )
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -framework OpenGL")
ENDIF()

# Installation
IF (WIN32)
	INSTALL (TARGETS ${CORE_LIBRARY_NAME} RUNTIME DESTINATION .)
ELSE ()
	INSTALL (TARGETS ${CORE_LIBRARY_NAME} LIBRARY DESTINATION .)
ENDIF(WIN32)

IF (MSVC)
	# Apply file grouping based on regular expressions for Visual Studio IDE.
	SOURCE_GROUP("UI Files" REGULAR_EXPRESSION "[.](ui|qrc)$")
ENDIF()
