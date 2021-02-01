SET( DEPENDENCIES_LIBRARIES
	iAcharts	# also pulls in required iAqthelper
	iAobjectvis
	iArenderer  # for iARendererImpl and iARendererManager
	iAslicer    # also pulls in iAcore
	
)
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	MaximumDistance
)
SET( DEPENDENCIES_MODULES
	FeatureAnalyzerComputation  # mainly for FeatureAnalyzerHelper
	FeatureScout
)
