SET( MODULE_DESCRIPTION_OUT
	"Use GPU based gradient anisotropic diffusion\n Requires: ITK GPU"
	PARENT_SCOPE
)
SET( MODULE_DEFAULT_OPTION_VALUE_OUT OFF  PARENT_SCOPE)

IF (Module_GPU_GradientAnisotropicDiffusion AND "${ITKGPUCommon_LIBRARY_DIRS}" STREQUAL "")
	MESSAGE(SEND_ERROR "Module_GPU_GradientAnisotropicDiffusion requires ITK to be built with GPU support, but the used ITK library has it disabled. Please build ITK libraries with ITK_USE_GPU enabled!")
ENDIF()