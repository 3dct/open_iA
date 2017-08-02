SET( MODULE_DESCRIPTION_OUT
	"Random Walker"
	PARENT_SCOPE
)
SET( MODULE_DEFAULT_OPTION_VALUE_OUT OFF  PARENT_SCOPE)

IF (BUILD_TESTING AND Module_SegmentationRandomWalker)
	
	get_filename_component(CoreSrcDir "../../core/src" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_filename_component(CoreBinDir "../../core" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	INCLUDE_DIRECTORIES(${CoreSrcDir}/src)  # for pch.h, iAFunction.h, ...
	INCLUDE_DIRECTORIES(${CoreBinDir})      # for open_iA_Core_export.h
	
	ADD_EXECUTABLE(ImageGraphTest iAImageGraphTest.cpp iAImageGraph.cpp ${CoreSrcDir}/iAImageCoordinate.cpp)
	ADD_EXECUTABLE(DistanceMeasureTest iADistanceMeasureTest.cpp iASpectraDistanceImpl.cpp ${CoreSrcDir}/iASpectrumType.cpp)

	TARGET_LINK_LIBRARIES(ImageGraphTest ${QT_LIBRARIES})
	TARGET_LINK_LIBRARIES(DistanceMeasureTest ${QT_LIBRARIES})

	target_compile_definitions(ImageGraphTest PRIVATE NO_DLL_LINKAGE)
	target_compile_definitions(DistanceMeasureTest PRIVATE NO_DLL_LINKAGE)

	ADD_TEST(NAME ImageGraphTest COMMAND ImageGraphTest)
	ADD_TEST(NAME DistanceMeasureTest COMMAND DistanceMeasureTest)

ENDIF (BUILD_TESTING AND Module_SegmentationRandomWalker)

