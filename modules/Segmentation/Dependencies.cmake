SET( DEPENDENCIES_LIBRARIES
	iAbase
	ITKClassifiers      # for ScalarImageKmeansImageFilter
	ITKLevelSets        # for LaplacianSegmentationLevelSetImageFilter, ...
	ITKRegionGrowing    # for ConfidenceConnectedImageFilter, ...
	ITKStatistics       # for EuclideanDistanceMetric
	ITKThresholding     # for BinaryThresholdImageFilter
	ITKWatersheds       # for MorphologicalWatershedImageFilter, ...
)
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
