SET( DEPENDENCIES_LIBRARIES
	iAcharts
	iAcore
)
#SET( DEPENDENCIES_ITK_MODULES
#	ITKAnisotropicSmoothing    # for CurvatureAnisotropicDiffusionImageFilter, GradientAnisotropicDiffusionImageFilter
#	ITKConnectedComponents     # for ...ConnectedComponentImageFilter, RelabelComponentImageFilter
#	ITKCurvatureFlow           # for CurvatureFlowImageFilter
#	ITKFiniteDifference        # for DenseFiniteDifferenceImageFilter (dependency of AnisotropicDiffusionImageFilter)
#	ITKImageFeature            # for BilateralImageFilter
#	ITKImageGradient           # for GradientMagnitudeImageFilter
#	ITKImageLabel              # for BinaryContourImageFilter
#	ITKImageSources            # for GaussianImageSource, dependency of BilateralImageFilter
#	ITKLabelMap                # for LabelImageToShapeLabelMapFilter
#	ITKMathematicalMorphology  # for RegionalMinimaImageFilter
#	ITKRegionGrowing           # for ConfidenceConnectedImageFilter, ConnectedThresholdImageFilter
#	ITKReview                  # for LabelGeometryImageFilter, RobustAutomaticThresholdImageFilter
#	ITKSmoothing               # for MedianImageFilter, RecursiveGaussianImageFilter
#	ITKStatistics              # for ImageToHistogramFilter
#	ITKThresholding            # for BinaryThresholdImageFilter
#	ITKTransform               # for AffineTransform (dependency of LabelGeometryImageFilter)
#	ITKWatersheds              # for MorphologicalWatershedImageFilter
#)
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	MaximumDistance
)
