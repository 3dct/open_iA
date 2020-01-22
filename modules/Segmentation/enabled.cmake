IF (openiA_TESTING_ENABLED)
	get_filename_component(CoreSrcDir "../core/src" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_filename_component(CoreBinDir "../core" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	ADD_EXECUTABLE(ImageGraphTest Segmentation/iAImageGraphTest.cpp Segmentation/iAImageGraph.cpp ${CoreSrcDir}/iAImageCoordinate.cpp)
	ADD_EXECUTABLE(DistanceMeasureTest Segmentation/iADistanceMeasureTest.cpp Segmentation/iAVectorDistanceImpl.cpp Segmentation/iAVectorArrayImpl.cpp Segmentation/iAVectorTypeImpl.cpp ${CoreSrcDir}/iAImageCoordinate.cpp)
	TARGET_LINK_LIBRARIES(ImageGraphTest PRIVATE ${QT_LIBRARIES})
	TARGET_LINK_LIBRARIES(DistanceMeasureTest PRIVATE ${QT_LIBRARIES} ${VTK_LIBRARIES})
	TARGET_INCLUDE_DIRECTORIES(ImageGraphTest PRIVATE ${CoreSrcDir} ${CoreBinDir})
	TARGET_INCLUDE_DIRECTORIES(DistanceMeasureTest PRIVATE ${CoreSrcDir} ${CoreBinDir})
	TARGET_COMPILE_DEFINITIONS(ImageGraphTest PRIVATE NO_DLL_LINKAGE)
	TARGET_COMPILE_DEFINITIONS(DistanceMeasureTest PRIVATE NO_DLL_LINKAGE)
	ADD_TEST(NAME ImageGraphTest COMMAND ImageGraphTest)
	ADD_TEST(NAME DistanceMeasureTest COMMAND DistanceMeasureTest)
	IF (MSVC)
		STRING(REGEX REPLACE "/" "\\\\" QT_WIN_DLL_DIR ${QT_LIB_DIR})
		SET_TESTS_PROPERTIES(ImageGraphTest PROPERTIES ENVIRONMENT "PATH=${QT_WIN_DLL_DIR};$ENV{PATH}")
		SET_TESTS_PROPERTIES(DistanceMeasureTest PROPERTIES ENVIRONMENT "PATH=${QT_WIN_DLL_DIR};$ENV{PATH}")
	ENDIF()

	IF (openiA_USE_IDE_FOLDERS)
		SET_PROPERTY(TARGET ImageGraphTest PROPERTY FOLDER "Tests")
		SET_PROPERTY(TARGET DistanceMeasureTest PROPERTY FOLDER "Tests")
	ENDIF()
ENDIF ()