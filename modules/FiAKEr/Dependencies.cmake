SET( DEPENDENCIES_LIBRARIES
	iAcharts      # also pulls in required iAqthelper
	iAcore
	iAobjectvis
	iArenderer    # for iARendererManager
	${VTK_LIB_PREFIX}FiltersExtraction           # for vtkExtractGeometry used iASelectionInteractorStyle
)
