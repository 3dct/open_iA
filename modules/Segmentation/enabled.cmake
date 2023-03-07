if (openiA_TESTING_ENABLED)
	get_filename_component(CoreSrcDir "../libs" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_filename_component(CoreBinDir "../libs" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	add_executable(ImageGraphTest Segmentation/iAImageGraphTest.cpp Segmentation/iAImageGraph.cpp ${CoreSrcDir}/base/iAImageCoordinate.cpp)
	add_executable(DistanceMeasureTest Segmentation/iADistanceMeasureTest.cpp Segmentation/iAVectorDistanceImpl.cpp Segmentation/iAVectorArrayImpl.cpp Segmentation/iAVectorTypeImpl.cpp ${CoreSrcDir}/base/iAImageCoordinate.cpp)
	target_link_libraries(ImageGraphTest PRIVATE Qt${QT_VERSION_MAJOR}::Core)
	target_link_libraries(DistanceMeasureTest PRIVATE Qt${QT_VERSION_MAJOR}::Core)
	set(VTK_REQUIRED_LIBS
		CommonCore        # for vtkSmartPointer
		CommonDataModel   # for vtkImageData
	)
	ADD_VTK_LIBRARIES(DistanceMeasureTest "PRIVATE" "${VTK_REQUIRED_LIBS}")
	#set(ITK_REQUIRED_LIBS
	#	ITKCommon
	#	ITKVNL             # drawn in by itkVector
	#)
	# version check needs to be verified, not sure if these dependencies were introduced exactly with v5.0.0
	# currently known: they are required in ITK 4.10.0, but not in 5.1.0
	#if (ITK_VERSION VERSION_LESS "5.0.0")
	#	list(APPEND ITK_REQUIRED_LIBS ITKKWSys)
	#endif()
	#ADD_LEGACY_LIBRARIES(DistanceMeasureTest "" "PRIVATE" "${ITK_REQUIRED_LIBS}")
	target_include_directories(ImageGraphTest PRIVATE ${CoreSrcDir}/base  ${CoreBinDir})
	target_include_directories(DistanceMeasureTest PRIVATE ${CoreSrcDir}/base ${CoreBinDir} ${CMAKE_CURRENT_BINARY_DIR})
	target_compile_definitions(ImageGraphTest PRIVATE NO_DLL_LINKAGE)
	target_compile_definitions(DistanceMeasureTest PRIVATE NO_DLL_LINKAGE)
	add_test(NAME ImageGraphTest COMMAND ImageGraphTest)
	add_test(NAME DistanceMeasureTest COMMAND DistanceMeasureTest)
	if (MSVC)
		set_tests_properties(ImageGraphTest PROPERTIES ENVIRONMENT "PATH=${TestEnvPath}")
		set_tests_properties(DistanceMeasureTest PROPERTIES ENVIRONMENT "PATH=${TestEnvPath}")
		set_target_properties(ImageGraphTest PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${WinDLLPaths};$ENV{PATH}")
		set_target_properties(DistanceMeasureTest PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${WinDLLPaths};$ENV{PATH}")
	endif()

	if (openiA_USE_IDE_FOLDERS)
		set_property(TARGET ImageGraphTest PROPERTY FOLDER "Tests")
		set_property(TARGET DistanceMeasureTest PROPERTY FOLDER "Tests")
	endif()
endif()

if (EIGEN3_FOUND)
	target_compile_definitions(Segmentation PRIVATE USE_EIGEN)
endif()
