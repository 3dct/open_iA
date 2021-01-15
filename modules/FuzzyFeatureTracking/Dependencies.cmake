SET( DEPENDENCIES_LIBRARIES
	iAcore
	iArenderer    # for iARendererManager
	${VTK_LIB_PREFIX}ChartsCore  # for vtkAxis
	${VTK_LIB_PREFIX}RenderingContext2D    # for vtkContextActor, vtkContextScene, vtkContextTransform
	${VTK_LIB_PREFIX}RenderingContext${VTK_RENDERING_BACKEND}    # for implementation of RenderingContext2D (?)
	${VTK_LIB_PREFIX}ViewsContext2D        # for vtkContextView, vtkContextInteractorStyle
	${VTK_LIB_PREFIX}ViewsInfovis    # for vtkGraphItem
)
