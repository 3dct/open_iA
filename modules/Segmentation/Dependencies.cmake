SET( DEPENDENCIES_LIBRARIES
	iAbase
)
#SET( DEPENDENCIES_ITK_MODULES
#	ITKClassifiers               # for ScalarImageKmeansImageFilter
#	ITKConnectedComponents       # for ConnectedComponentImageFilter (dependency of MorphologicalWatershedImageFilter)
#	ITKDistanceMap               # for DanielssonDistanceMapImageFilter (dependency of CannySegmentationLevelSetFilter)
#	ITKFiniteDifference          # for FiniteDifferenceImageFilter (dependency of SparseFieldLevelSetImageFilter)
#	ITKImageFeature              # for ZeroCrossingImageFilter
#	ITKImageGradient             # for GradientMagnitudeImageFilter
#	ITKImageLabel                # for ScanlineFilterCommon (dependency of ConnectedComponentImageFilter)
#	ITKLevelSets                 # for LaplacianSegmentationLevelSetImageFilter, ...
#	ITKMathematicalMorphology    # for BinaryBallStructuringElement, FlatStructuringElement, RegionalMinimaImageFilter
#	ITKRegionGrowing             # for ConfidenceConnectedImageFilter, ...
#	ITKReview                    # for RobustAutomaticThresholdImageFilter
#	ITKSmoothing                 # for DiscreteGaussianImageFilter (dependency of CannyEdgeDetectionImageFilter)
#	ITKStatistics                # for EuclideanDistanceMetric
#	ITKThresholding              # for BinaryThresholdImageFilter
#	ITKWatersheds                # for MorphologicalWatershedImageFilter, WatershedImageFilter, ...
#)
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	AdaptiveOtsuThreshold
	MaximumDistance
	RemovePeaksOtsu
	FuzzyCMeans
)
IF (EIGEN3_FOUND)
	SET( DEPENDENCIES_INCLUDE_DIRS
		${EIGEN3_INCLUDE_DIR}
	)
ENDIF()
