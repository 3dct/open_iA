SET( DEPENDENCIES_LIBRARIES
	iAbase
)
SET( DEPENDENCIES_ITK_MODULES
	ITKImageFeature    # for HessianRecursiveGaussianImageFilter
	ITKSmoothing
)
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	RemovePeaksOtsu    # for itkFHWRescaleIntensityImageFilter
)
