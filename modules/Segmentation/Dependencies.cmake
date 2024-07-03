set(DEPENDENCIES_LIBRARIES
	iA::base
)
if (OpenMP_CXX_FOUND)
	list(APPEND DEPENDENCIES_LIBRARIES OpenMP::OpenMP_CXX)
endif()
#set(DEPENDENCIES_ITK_MODULES
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
set(DEPENDENCIES_IA_TOOLKIT_DIRS
	FuzzyCMeans
	MaximumDistance
)
if (EIGEN3_FOUND)
	set(DEPENDENCIES_INCLUDE_DIRS
		${EIGEN3_INCLUDE_DIR}
	)
endif()
