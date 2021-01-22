SET( DEPENDENCIES_LIBRARIES
	iAcore
	iArenderer    # for iARendererManager
)
SET( DEPENDENCIES_VTK_MODULES
	ChartsCore  # for vtkAxis
	RenderingContext2D    # for vtkContextActor, vtkContextScene, vtkContextTransform
	RenderingContext${VTK_RENDERING_BACKEND}    # for implementation of RenderingContext2D (?)
	ViewsContext2D        # for vtkContextView, vtkContextInteractorStyle
	ViewsInfovis    # for vtkGraphItem
)
