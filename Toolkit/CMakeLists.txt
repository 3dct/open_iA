IF(Module_GPU_DreamcasterTool)
	IF (CMAKE_COMPILER_IS_GNUCXX)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic")
	ELSEIF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	ENDIF()

	IF (NOT OPENCL_FOUND)
		MESSAGE(SEND_ERROR "Dreamcaster requires OpenCL_FOUND to be true!")
	ENDIF()

	IF(open_iA_GPU_USING_CUDA)
		INCLUDE(${CMAKE_ROOT}/Modules/FindCUDA.cmake)
	ENDIF(open_iA_GPU_USING_CUDA)

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

	SET(DREAMCASTER_EXECUTABLE_PATH "${EXECUTABLE_OUTPUT_PATH}")

	ADD_LIBRARY (Dreamcaster ${DREAMCASTER_SRCS})

	get_filename_component(CoreSrcDir "../core" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_filename_component(CoreBinDir "../core" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	target_compile_definitions(Dreamcaster PRIVATE NO_DLL_LINKAGE)
	TARGET_INCLUDE_DIRECTORIES(Dreamcaster PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Dreamcaster)
	TARGET_INCLUDE_DIRECTORIES(Dreamcaster PRIVATE ${CoreSrcDir}/src ${OPENCL_INCLUDE_DIRS} ${CoreBinDir} ${CMAKE_CURRENT_SOURCE_DIR}/OpenCL)
	SET_PROPERTY(TARGET Dreamcaster PROPERTY FOLDER "Modules")
ENDIF(Module_GPU_DreamcasterTool)
