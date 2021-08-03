SET( DEPENDENCIES_LIBRARIES
	iAcharts      # also pulls in required iAqthelper
	iAguibase
	iAobjectvis
	iArenderer    # for iARendererViewSync
	Segmentation  # required for distance measures; maybe move these to core?
)
SET( DEPENDENCIES_VTK_MODULES
	FiltersExtraction           # for vtkExtractGeometry used iASelectionInteractorStyle
)
