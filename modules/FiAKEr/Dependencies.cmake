set(DEPENDENCIES_LIBRARIES
	iA::charts      # also pulls in required iAqthelper
	iA::guibase
	iA::objectvis
	iA::renderer    # for iARendererViewSync
)
set(DEPENDENCIES_MODULES
	Segmentation  # required for distance measures; maybe move these to core?
)
set(DEPENDENCIES_VTK_MODULES
	FiltersExtraction           # for vtkExtractGeometry used iASelectionInteractorStyle
	FiltersPoints               # for vtkSignedDistance, vtkPCANormalEstimation, vtkExtractSurface
)
