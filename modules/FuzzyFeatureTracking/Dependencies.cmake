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
# for VTK < 9 we have to use VTK_USE_FILE anyway for module autoinitialization
#if (VTK_VERSION VERSION_LESS "9.0.0")
#	list(APPEND DEPENDENCIES_VTK_MODULES
#		ViewsCore     # for vtkViewsCoreModule.h, required by vtkViewsInfovisModule.h
#	)
#endif()
