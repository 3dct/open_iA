SET( MODULE_DESCRIPTION_OUT
	"Use Modality Explorer:\nBasic Tools for exploring images of the same object acquired through different modalities"
	PARENT_SCOPE
)
SET( MODULE_DEFAULT_OPTION_VALUE_OUT OFF  PARENT_SCOPE)

IF (WIN32 AND MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
ENDIF (WIN32 AND MSVC)

