SET( DEPENDENCIES_LIBRARIES
	iAcharts      # also pulls in required iAqthelper
	iAcore
	iAobjectvis
	iArenderer    # for iARendererManager
	Segmentation
)


# Segmentation module required for distance measures
# maybe move these to core?