SET( DEPENDENCIES_LIBRARIES
	iAcharts      # also pulls in required iAqthelper
	iAslicer      # for access to iASlicerImpl, also pulls in required iAcore
	${VTK_LIB_PREFIX}ChartsCore  # for vtkAxis
	${VTK_LIB_PREFIX}RenderingContext2D    # for vtkContextScene
	${VTK_LIB_PREFIX}RenderingContext${VTK_RENDERING_BACKEND}    # for implementation of RenderingContext2D (?)
	${VTK_LIB_PREFIX}ViewsContext2D        # for vtkContextView
)
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	LabelVoting
)
SET (DEPENDENCIES_MODULES
	MetaFilters   # for iASingleResult, iASamplingResults
)
