IF(Module_GPU_DreamcasterTool)
	IF (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic")
	ELSEIF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	ENDIF()

	IF (NOT OPENCL_FOUND)
		MESSAGE(SEND_ERROR "Dreamcaster requires OpenCL_FOUND to be true!")
	ENDIF()

	#.cpp sources
	SET( DREAMCASTER_SRCS
		Dreamcaster/iADreamCaster.cpp
		Dreamcaster/iAPaintWidget.cpp
		Dreamcaster/iAStabilityWidget.cpp
		Dreamcaster/iAComparisonAndWeighting.cpp
		Dreamcaster/dlg_histogram_simple.cpp

		Dreamcaster/iADreamCaster.h
		Dreamcaster/iAPaintWidget.h
		Dreamcaster/iAStabilityWidget.h
		Dreamcaster/iAComparisonAndWeighting.h
		Dreamcaster/dlg_histogram_simple.h

		Dreamcaster/raycast/src/iADreamCasterCommon.cpp
		Dreamcaster/raycast/src/iARaytracer.cpp
		Dreamcaster/raycast/src/iAScene.cpp
		Dreamcaster/raycast/src/iASTLLoader.cpp
		Dreamcaster/raycast/src/iAScreenBuffer.cpp
		Dreamcaster/raycast/src/iAPlot3DVtk.cpp

		Dreamcaster/raycast/include/iABSPTree.h
		Dreamcaster/raycast/include/iADreamCasterCommon.h
		Dreamcaster/raycast/include/iACutFigList.h
		Dreamcaster/raycast/include/iADataFormat.h
		Dreamcaster/raycast/include/iARaytracer.h
		Dreamcaster/raycast/include/iAScene.h
		Dreamcaster/raycast/include/iASTLLoader.h
		Dreamcaster/raycast/include/iAScreenBuffer.h
		Dreamcaster/raycast/include/iAPlot3DVtk.h
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

	# build ui_XXX files from the XML-style .ui files
	QT5_WRAP_UI(DREAMCASTER_SRCS ${DREAMCASTER_UI})
	# this appends qrc qt4 resources to the project
	QT5_ADD_RESOURCES( DREAMCASTER_SRCS ${DREAMCASTER_QRC} ) #resources have to be added to executables!

	SET(DREAMCASTER_EXECUTABLE_PATH "${EXECUTABLE_OUTPUT_PATH}")

	ADD_LIBRARY (Dreamcaster ${DREAMCASTER_SRCS} ${DREAMCASTER_QRC})

	get_filename_component(CoreSrcDir "../core" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_filename_component(CoreBinDir "../core" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	target_compile_definitions(Dreamcaster PRIVATE NO_DLL_LINKAGE)
	TARGET_INCLUDE_DIRECTORIES(Dreamcaster PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Dreamcaster)
	TARGET_INCLUDE_DIRECTORIES(Dreamcaster PRIVATE ${CoreSrcDir}/src ${OPENCL_INCLUDE_DIRS} ${CoreBinDir} ${CMAKE_CURRENT_SOURCE_DIR}/OpenCL)
	
	IF (openiA_USE_IDE_FOLDERS)
		SET_PROPERTY(TARGET Dreamcaster PROPERTY FOLDER "Libraries")
	ENDIF()

ENDIF(Module_GPU_DreamcasterTool)
