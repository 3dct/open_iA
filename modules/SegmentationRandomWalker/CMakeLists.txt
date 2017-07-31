SET( MODULE_DESCRIPTION_OUT
	"Random Walker"
	PARENT_SCOPE
)
SET( MODULE_DEFAULT_OPTION_VALUE_OUT OFF  PARENT_SCOPE)

IF (BUILD_TESTING AND Module_SegmentationRandomWalker)

	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/core/src)  # for pch.h, iAFunction.h, ...
	INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/core)  # for open_iA_Core_export.h
	
	ADD_EXECUTABLE(ImageGraphTest iAImageGraphTest.cpp iAImageGraph.cpp ${CMAKE_SOURCE_DIR}/core/src/iAImageCoordinate.cpp)
	ADD_EXECUTABLE(DistanceMeasureTest iADistanceMeasureTest.cpp iASpectraDistanceImpl.cpp ${CMAKE_SOURCE_DIR}/core/src/iASpectrumType.cpp)

	TARGET_LINK_LIBRARIES(ImageGraphTest ${QT_LIBRARIES})
	TARGET_LINK_LIBRARIES(DistanceMeasureTest ${QT_LIBRARIES})
	
	target_compile_definitions(ImageGraphTest PRIVATE NO_DLL_LINKAGE)
	target_compile_definitions(DistanceMeasureTest PRIVATE NO_DLL_LINKAGE)

	ADD_TEST(NAME ImageGraphTest COMMAND ImageGraphTest)
	ADD_TEST(NAME DistanceMeasureTest COMMAND DistanceMeasureTest)

ENDIF (BUILD_TESTING AND Module_SegmentationRandomWalker)

