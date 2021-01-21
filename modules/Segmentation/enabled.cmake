IF (openiA_TESTING_ENABLED)
	get_filename_component(CoreSrcDir "../libs" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_filename_component(CoreBinDir "../libs" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	ADD_EXECUTABLE(ImageGraphTest Segmentation/iAImageGraphTest.cpp Segmentation/iAImageGraph.cpp ${CoreSrcDir}/base/iAImageCoordinate.cpp)
	ADD_EXECUTABLE(DistanceMeasureTest Segmentation/iADistanceMeasureTest.cpp Segmentation/iAVectorDistanceImpl.cpp Segmentation/iAVectorArrayImpl.cpp Segmentation/iAVectorTypeImpl.cpp ${CoreSrcDir}/base/iAImageCoordinate.cpp)
	TARGET_LINK_LIBRARIES(ImageGraphTest PRIVATE Qt5::Core)
	TARGET_LINK_LIBRARIES(DistanceMeasureTest PRIVATE Qt5::Core
		${VTK_LIB_PREFIX}CommonCore      # for vtkSmartPointer
		${VTK_LIB_PREFIX}CommonDataModel # for vtkImageData
	)
	SET(ITK_REQUIRED_LIBS
		ITKCommon
		ITKVNL             # drawn in by itkVector
	)
	FOREACH(itklib ${ITK_REQUIRED_LIBS})
		MESSAGE(STATUS "${itklib} - lib: ${${itklib}_LIBRARIES}, include: ${${itklib}_INCLUDE_DIRS}")
		TARGET_LINK_LIBRARIES(DistanceMeasureTest PUBLIC ${${itklib}_LIBRARIES})
		TARGET_INCLUDE_DIRECTORIES(DistanceMeasureTest PUBLIC ${${itklib}_INCLUDE_DIRS})
	ENDFOREACH()
	TARGET_INCLUDE_DIRECTORIES(ImageGraphTest PRIVATE ${CoreSrcDir}/base  ${CoreBinDir})
	TARGET_INCLUDE_DIRECTORIES(DistanceMeasureTest PRIVATE ${CoreSrcDir}/base ${CoreBinDir})
	TARGET_COMPILE_DEFINITIONS(ImageGraphTest PRIVATE NO_DLL_LINKAGE)
	TARGET_COMPILE_DEFINITIONS(DistanceMeasureTest PRIVATE NO_DLL_LINKAGE)
	ADD_TEST(NAME ImageGraphTest COMMAND ImageGraphTest)
	ADD_TEST(NAME DistanceMeasureTest COMMAND DistanceMeasureTest)
	IF (MSVC)
		STRING(REGEX REPLACE "/" "\\\\" QT_WIN_DLL_DIR ${QT_LIB_DIR})
		SET_TESTS_PROPERTIES(ImageGraphTest PROPERTIES ENVIRONMENT "PATH=${QT_WIN_DLL_DIR};$ENV{PATH}")
		SET_TESTS_PROPERTIES(DistanceMeasureTest PROPERTIES ENVIRONMENT "PATH=${QT_WIN_DLL_DIR};$ENV{PATH}")
		SET_TARGET_PROPERTIES(ImageGraphTest PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${QT_WIN_DLL_DIR};$ENV{PATH}")
		SET_TARGET_PROPERTIES(DistanceMeasureTest PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${QT_WIN_DLL_DIR};$ENV{PATH}")
	ENDIF()

	IF (openiA_USE_IDE_FOLDERS)
		SET_PROPERTY(TARGET ImageGraphTest PROPERTY FOLDER "Tests")
		SET_PROPERTY(TARGET DistanceMeasureTest PROPERTY FOLDER "Tests")
	ENDIF()
ENDIF ()

IF (EIGEN3_FOUND)
	TARGET_COMPILE_DEFINITIONS(Segmentation PRIVATE USE_EIGEN)
ENDIF()
