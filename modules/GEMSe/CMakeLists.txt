SET( MODULE_DESCRIPTION_OUT
	"GEMSe:\nVisualization-Guided Exploration of Multi-channel Segmentation algorithms"
	PARENT_SCOPE
)
SET( MODULE_DEFAULT_OPTION_VALUE_OUT OFF  PARENT_SCOPE)

IF (BUILD_TESTING AND Module_GEMSe)
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/core/src)  # for pch.h, iAFunction.h, ...
	INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/core)  # for open_iA_Core_export.h
	ADD_EXECUTABLE(StringHelperTest iAStringHelperTest.cpp ${CMAKE_SOURCE_DIR}/core/src/iAStringHelper.cpp)
	TARGET_LINK_LIBRARIES(StringHelperTest ${QT_LIBRARIES})
	target_compile_definitions(StringHelperTest PRIVATE NO_DLL_LINKAGE)
	ADD_TEST(NAME StringHelperTest COMMAND StringHelperTest)
ENDIF (BUILD_TESTING AND Module_GEMSe)