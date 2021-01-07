SET (DEPENDENCIES_IA_TOOLKIT_DIRS
	MaximumDistance
)
SET( DEPENDENCIES_LIBRARIES
	iAcharts
	iAobjectvis
	# iAqthelper    # pulled in by charts automatically
)
SET (DEPENDENCIES_MODULES
	FeatureAnalyzerComputation  # mainly for FeatureAnalyzerHelper
	FeatureScout
)

SET (DEPENDENCIES_BASE_LIBRARY iAslicer) # for access to iASlicerImpl