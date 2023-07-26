set(DEPENDENCIES_LIBRARIES
	iA::guibase
	iA::renderer    # for iARendererViewSync
)
set(DEPENDENCIES_VTK_MODULES
	ChartsCore  # for vtkAxis
	RenderingContext2D    # for vtkContextActor, vtkContextScene, vtkContextTransform
	RenderingContextOpenGL2 # for implementation of RenderingContext2D (?)
	ViewsContext2D        # for vtkContextView, vtkContextInteractorStyle
	ViewsInfovis    # for vtkGraphItem
)
