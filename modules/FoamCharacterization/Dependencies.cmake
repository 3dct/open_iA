SET( DEPENDENCIES_LIBRARIES
	iAcore
	iAqthelper
)
SET( DEPENDENCIES_ITK_MODULES
	ITKAnisotropicSmoothing    # for GradientAnisotropicDiffusionImageFilter
	ITKDenoising               # for PatchBasedDenoisingBaseImageFilter
	ITKDistanceMap             # for itkDanielssonDistanceMapImageFilter
	ITKFiniteDifference        # for DenseFiniteDifferenceImageFilter, dependency of AnisotropicDiffusionImageFilter
	ITKImageAdaptors           # for NthElementImageAdaptor (dependency of HessianRecursiveGaussianImageFilter, PatchBasedDenoisingImageFilter)
	ITKImageGradient           # for GradientMagnitudeImageFilter
	ITKLabelMap                # for LabelMap
	ITKReview                  # for LabelGeometryImageFilter
	ITKSmoothing               # for DiscreteGaussianImageFilter, MedianImageFilter
	ITKStatistics              # for Sample
	ITKTransform               # for AffineTransform, dependency of LabelGeometryImageFilter
	ITKWatersheds              # for WatershedImageFilter
)
