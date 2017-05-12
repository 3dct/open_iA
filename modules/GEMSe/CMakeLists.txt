SET( MODULE_DESCRIPTION_OUT
	"GEMSe:\nVisualization-Guided Exploration of Multi-channel Segmentation algorithms"
	PARENT_SCOPE
)
SET( MODULE_DEFAULT_OPTION_VALUE_OUT OFF  PARENT_SCOPE)

IF (BUILD_TESTING AND Module_GEMSe)
	INCLUDE_DIRECTORIES(${open_iA_core_SOURCE_DIR}/src)  # for pch.h, iAFunction.h, ...
	INCLUDE_DIRECTORIES(${open_iA_core_BINARY_DIR})  # for open_iA_Core_export.h
	ADD_EXECUTABLE(StringHelperTest iAStringHelperTest.cpp ${open_iA_core_SOURCE_DIR}/src/iAStringHelper.cpp)
	TARGET_LINK_LIBRARIES(StringHelperTest ${QT_LIBRARIES})
	target_compile_definitions(StringHelperTest PRIVATE NO_DLL_LINKAGE)
	ADD_TEST(NAME StringHelperTest COMMAND StringHelperTest)
ENDIF (BUILD_TESTING AND Module_GEMSe)