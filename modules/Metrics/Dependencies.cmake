set(DEPENDENCIES_LIBRARIES
	iA::charts         # also pulls in required iAqthelper
	iA::guibase
)
#set(DEPENDENCIES_ITK_MODULES
#	ITKImageAdaptors        # for NthElementImageAdaptor (dependency of GradientRecursiveGaussianImageFilter)
#	ITKImageCompose         # for JoinImageFilter
#	ITKImageFunction        # for ...ImageFunction (dependency of ImageToImageMetric
#	ITKImageGradient        # for GradientRecursiveGaussianImageFilter (dependency of ImageToImageMetric)
#	ITKOptimizers           # for SingleValuedCostFunction (dependency of ImageToImageMetric)
#	ITKRegistrationCommon   # for NormalizedCorrelationImageToImageMetric, MeanSquaresImageToImageMetric
#	ITKSmoothing            # RecursiveGaussianImageFilter (dependency of GradientRecursiveGaussianImageFilter)
#	ITKSpatialObjects       # for SpatialObject (dependency of ImageToImageMetric)
#	ITKStatistics           # for ImageToHistogramFilter
#	ITKTransform            # for BSplineBaseTransform (dependency of ImageToImageMetric)
#)
