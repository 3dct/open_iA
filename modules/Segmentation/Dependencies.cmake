# Toolkit directories
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	AdaptiveOtsuThreshold
	MaximumDistance
	RemovePeaksOtsu
	FuzzyCMeans
)

IF (EIGEN3_FOUND)
	SET(DEPENDENCIES_INCLUDE_DIRS ${EIGEN3_INCLUDE_DIR})
ENDIF()