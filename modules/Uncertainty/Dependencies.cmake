SET( DEPENDENCIES_LIBRARIES
	iAcharts      # also pulls in required iAqthelper
	iAslicer      # for access to iASlicerImpl, also pulls in required iAcore
)
SET( DEPENDENCIES_VTK_MODULES
	ChartsCore  # for vtkAxis
	RenderingContext2D    # for vtkContextScene
	RenderingContext${VTK_RENDERING_BACKEND}    # for implementation of RenderingContext2D (?)
	ViewsContext2D        # for vtkContextView
)
IF (VTK_VERSION VERSION_LESS "9.0.0")
	LIST(APPEND DEPENDENCIES_VTK_MODULES
		ViewsCore     # for vtkViewsCoreModule.h, required by vtkViewsContext2DModule.h (iAScatterPlotView.cpp)
	)
ENDIF()
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	Entropy
)
SET( DEPENDENCIES_MODULES
	MetaFilters   # for iASingleResult, iASamplingResults
)
