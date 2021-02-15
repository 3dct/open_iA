SET( DEPENDENCIES_LIBRARIES
	iAbase
)
if (OPENCL_FOUND)
	LIST(APPEND DEPENDENCIES_LIBRARIES OpenCL)    # not ideal - only required for TARGET_VERSION defines
endif()
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	RemovePeaksOtsu    # for itkFHWRescaleIntensityImageFilter
)
