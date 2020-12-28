FILE( GLOB_RECURSE open_iA_gui_SRCS "*.cpp" "*.h" )
FILE( GLOB_RECURSE open_iA_gui_UI "*.ui" )
SET( open_iA_gui_QRC open_iA_gui.qrc)

QT5_WRAP_UI( open_iA_gui_SRCS ${open_iA_gui_UI})
QT5_ADD_RESOURCES( open_iA_gui_SRCS ${open_iA_gui_QRC} )

SET (LIB_TYPE SHARED)
SET (GUI_LIBRARY_NAME open_iA_gui)

# Generate core dll file / shared object:
add_library(${GUI_LIBRARY_NAME} ${LIB_TYPE} ${open_iA_gui_SRCS} ${open_iA_gui_QRC})

TARGET_LINK_LIBRARIES(${GUI_LIBRARY_NAME} PUBLIC ${CORE_LIBRARY_NAME} charts)

INCLUDE(Modules)
MODULE_GENERATE_EXPORT_HEADER ( ${GUI_LIBRARY_NAME} )

if (VTK_MAJOR_VERSION GREATER 8)
	vtk_module_autoinit(TARGETS ${GUI_LIBRARY_NAME} MODULES ${VTK_LIBRARIES})
endif()

IF (openiA_PRECOMPILE)
	IF (CMAKE_MAJOR_VERSION GREATER 4 OR
	(CMAKE_MAJOR_VERSION EQUAL 3 AND CMAKE_MINOR_VERSION GREATER 15))
		INCLUDE(precompile.cmake)
		target_precompile_headers(${GUI_LIBRARY_NAME} PRIVATE ${OPENIA_PRECOMPILE_HEADERS})
	ENDIF()
ENDIF()

if (openiA_CHART_OPENGL)
	TARGET_COMPILE_DEFINITIONS(${GUI_LIBRARY_NAME} PUBLIC CHART_OPENGL)
endif()

# ${CMAKE_CURRENT_SOURCE_DIR}/src 
TARGET_INCLUDE_DIRECTORIES(${GUI_LIBRARY_NAME} PUBLIC
	# check why these includes are not automatically pulled in by target_link_library
	#${CMAKE_CURRENT_SOURCE_DIR}/../base
	#${CMAKE_CURRENT_SOURCE_DIR}/../charts
	#${CMAKE_CURRENT_SOURCE_DIR}/../src
	#${CMAKE_CURRENT_BINARY_DIR}/../base       # for export headers
	#${CMAKE_CURRENT_BINARY_DIR}/../charts
	#${CMAKE_CURRENT_BINARY_DIR}/../core
	${CMAKE_CURRENT_SOURCE_DIR}                # to make own header files available in libraries linking to this target
	${CMAKE_CURRENT_BINARY_DIR})

IF (MSVC)
	# Apply file grouping based on regular expressions for Visual Studio IDE.
	SOURCE_GROUP("UI Files" REGULAR_EXPRESSION "[.](ui|qrc)$")
ENDIF()

IF (openiA_USE_IDE_FOLDERS)
	SET_PROPERTY(TARGET ${GUI_LIBRARY_NAME} PROPERTY FOLDER "Libraries")
ENDIF()