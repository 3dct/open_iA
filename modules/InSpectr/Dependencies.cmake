SET( DEPENDENCIES_LIBRARIES
	iAcharts           # also pulls in required iAqthelper
	iAcore
	iArenderer
)
SET( DEPENDENCIES_ITK_MODULES
	ITKImageAdaptors         # for NthElementImageAdaptor (dependency of GradientRecursiveGaussianImageFilter)
	ITKImageGradient         # for GradientRecursiveGaussianImageFilter (dependency of PointSetToImageMetric)
	ITKOptimizers            # for SingleValuedCostFunction (dependency of ImageToImageMetric)
	ITKRegistrationCommon    # for MutualInformationImageToImageMetric
	ITKSmoothing             # RecursiveGaussianImageFilter (dependency of GradientRecursiveGaussianImageFilter)
	ITKSpatialObjects        # for SpatialObject (dependency of ImageToImageMetric)
	ITKTransform             # for IdentityTransform, TranslationTransform
)
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	FunctionalBoxplot
)
