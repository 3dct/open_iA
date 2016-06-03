CMAKE_MINIMUM_REQUIRED ( VERSION 3.0 )

PROJECT ( open_iA_toolkit LANGUAGES CXX )

SET ( PARENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )

#-----------------------------------------------------------------------------------------
# set the Output directory for the executable and library
IF(NOT LIBRARY_OUTPUT_PATH)
  SET (LIBRARY_OUTPUT_PATH ${open_iA_toolkit_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all libraries.")
ENDIF(NOT LIBRARY_OUTPUT_PATH)
IF(NOT EXECUTABLE_OUTPUT_PATH)
  SET (EXECUTABLE_OUTPUT_PATH ${open_iA_toolkit_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all executables.")
ENDIF(NOT EXECUTABLE_OUTPUT_PATH)
MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)

SET(Toolkit_LIBRARY_PATH "${LIBRARY_OUTPUT_PATH}")
SET(Toolkit_EXECUTABLE_PATH "${EXECUTABLE_OUTPUT_PATH}")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR})
INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/OpenCL/cl )

#-----------------------------------------------------------------------------------------
# DREAMCASTER
#-----------------------------------------------------------------------------------------
IF(Module_GPU_DreamcasterTool)
	IF (MSVC)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
	ENDIF (MSVC)
	IF (CMAKE_COMPILER_IS_GNUCXX)
	    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
	ENDIF(CMAKE_COMPILER_IS_GNUCXX)

	# set the Output directory for the executable and library
	IF(NOT LIBRARY_OUTPUT_PATH)
	  SET (LIBRARY_OUTPUT_PATH ${DREAMCASTER_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all libraries.")
	ENDIF(NOT LIBRARY_OUTPUT_PATH)
	IF(NOT EXECUTABLE_OUTPUT_PATH)
	  SET (EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin CACHE INTERNAL "Single output directory for building all executables.")
	ENDIF(NOT EXECUTABLE_OUTPUT_PATH)
	MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)

	IF(open_iA_GPU_USING_CUDA)
		INCLUDE(${CMAKE_ROOT}/Modules/FindCUDA.cmake)
	ENDIF(open_iA_GPU_USING_CUDA)

	INCLUDE_DIRECTORIES(${DREAMCASTER_SOURCE_DIR} ${DREAMCASTER_BINARY_DIR} ${open_iA_core_SOURCE_DIR}/src ${Toolkit_DIR}/cl)
	INCLUDE_DIRECTORIES( ${open_iA_core_BINARY_DIR} )

	#.cpp sources
	SET( DREAMCASTER_SRCS
		Dreamcaster/dreamcaster.cpp
		Dreamcaster/PaintWidget.cpp
		Dreamcaster/StabilityWidget.cpp
		Dreamcaster/ComparisonAndWeighting.cpp
		Dreamcaster/dlg_histogram_simple.cpp

		Dreamcaster/raycast/src/common.cpp
		Dreamcaster/raycast/src/raytracer.cpp
		Dreamcaster/raycast/src/scene.cpp
		Dreamcaster/raycast/src/STLLoader.cpp
		Dreamcaster/raycast/src/ScreenBuffer.cpp
		Dreamcaster/raycast/src/Plot3DVtk.cpp

		Dreamcaster/dreamcaster.h

		Dreamcaster/raycast/include/BSPTree.h
		Dreamcaster/raycast/include/common.h
		Dreamcaster/raycast/include/CutFigList.h
		Dreamcaster/raycast/include/DataFormat.h
		Dreamcaster/raycast/include/raytracer.h
		Dreamcaster/raycast/include/scene.h
		Dreamcaster/raycast/include/STLLoader.h
		Dreamcaster/raycast/include/ScreenBuffer.h
		Dreamcaster/raycast/include/Plot3DVtk.h

		Dreamcaster/ConfigFile/ConfigFile.cpp
	 )
	# *.ui files
	SET( DREAMCASTER_UI
		Dreamcaster/dream_caster4.ui
		Dreamcaster/LogWnd.ui
		Dreamcaster/results_dialog.ui
		Dreamcaster/settings.ui
	)
	# *.qrc files
	SET( DREAMCASTER_QRC
		Dreamcaster/dreamcaster.qrc
	)
	 #.cpp sources of client app
	SET( DREAMCASTER_CLIENT_SRCS
		Dreamcaster/Dreamcaster_client/main.cpp
	)

	# build ui_XXX files from the XML-style .ui files
	QT5_WRAP_UI(DREAMCASTER_SRCS ${DREAMCASTER_UI})
	# this appends qrc qt4 resources to the project
	QT5_ADD_RESOURCES( DREAMCASTER_CLIENT_SRCS ${DREAMCASTER_QRC} ) #resources have to be added to executables!

	INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

	SET(DREAMCASTER_LIBRARY_PATH "${LIBRARY_OUTPUT_PATH}")
	SET(DREAMCASTER_EXECUTABLE_PATH "${EXECUTABLE_OUTPUT_PATH}")
	#DREAMCASTER create library files (static libraries)
	ADD_LIBRARY (Dreamcaster ${DREAMCASTER_SRCS})
	
	target_compile_definitions(Dreamcaster PRIVATE NO_DLL_LINKAGE)
ENDIF(Module_GPU_DreamcasterTool)
