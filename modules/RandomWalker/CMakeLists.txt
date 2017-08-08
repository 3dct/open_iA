SET( MODULE_DESCRIPTION_OUT
	"Random Walker"
	PARENT_SCOPE
)
SET( MODULE_DEFAULT_OPTION_VALUE_OUT OFF  PARENT_SCOPE)

IF (BUILD_TESTING AND Module_SegmentationRandomWalker)
	get_filename_component(CoreSrcDir "../../core/src" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_filename_component(CoreBinDir "../../core" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	ADD_EXECUTABLE(ImageGraphTest iAImageGraphTest.cpp iAImageGraph.cpp ${CoreSrcDir}/iAImageCoordinate.cpp)
	ADD_EXECUTABLE(DistanceMeasureTest iADistanceMeasureTest.cpp iAVectorDistanceImpl.cpp iAVectorArrayImpl.cpp iAVectorTypeImpl.cpp ${CoreSrcDir}/iAImageCoordinate.cpp)
	TARGET_LINK_LIBRARIES(ImageGraphTest PRIVATE ${QT_LIBRARIES})
	TARGET_LINK_LIBRARIES(DistanceMeasureTest PRIVATE ${QT_LIBRARIES} ${VTK_LIBRARIES})
	TARGET_INCLUDE_DIRECTORIES(ImageGraphTest PRIVATE ${CoreSrcDir} ${CoreBinDir})
	TARGET_INCLUDE_DIRECTORIES(DistanceMeasureTest PRIVATE ${CoreSrcDir} ${CoreBinDir})
	TARGET_COMPILE_DEFINITIONS(ImageGraphTest PRIVATE NO_DLL_LINKAGE)
	TARGET_COMPILE_DEFINITIONS(DistanceMeasureTest PRIVATE NO_DLL_LINKAGE)
	ADD_TEST(NAME ImageGraphTest COMMAND ImageGraphTest)
	ADD_TEST(NAME DistanceMeasureTest COMMAND DistanceMeasureTest)
ENDIF (BUILD_TESTING AND Module_SegmentationRandomWalker)
