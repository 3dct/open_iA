set(DEPENDENCIES_LIBRARIES
	iA::charts           # also pulls in required iAqthelper
	iA::guibase
	iA::renderer
)
#set(DEPENDENCIES_ITK_MODULES
#	ITKImageAdaptors         # for NthElementImageAdaptor (dependency of GradientRecursiveGaussianImageFilter)
#	ITKImageGradient         # for GradientRecursiveGaussianImageFilter (dependency of PointSetToImageMetric)
#	ITKOptimizers            # for SingleValuedCostFunction (dependency of ImageToImageMetric)
#	ITKRegistrationCommon    # for MutualInformationImageToImageMetric
#	ITKSmoothing             # RecursiveGaussianImageFilter (dependency of GradientRecursiveGaussianImageFilter)
#	ITKSpatialObjects        # for SpatialObject (dependency of ImageToImageMetric)
#	ITKTransform             # for IdentityTransform, TranslationTransform
#)
set(DEPENDENCIES_IA_TOOLKIT_DIRS
	FunctionalBoxplot
)
