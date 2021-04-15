SET( DEPENDENCIES_LIBRARIES
	iAguibase
	iArenderer    # for iARendererManager
)
SET( DEPENDENCIES_VTK_MODULES
	ChartsCore  # for vtkAxis
	RenderingContext2D    # for vtkContextActor, vtkContextScene, vtkContextTransform
	RenderingContext${VTK_RENDERING_BACKEND}    # for implementation of RenderingContext2D (?)
	ViewsContext2D        # for vtkContextView, vtkContextInteractorStyle
	ViewsInfovis    # for vtkGraphItem
)
# for VTK < 9 we have to use VTK_USE_FILE anyway for module autoinitialization
#IF (VTK_VERSION VERSION_LESS "9.0.0")
#	LIST(APPEND DEPENDENCIES_VTK_MODULES
#		ViewsCore     # for vtkViewsCoreModule.h, required by vtkViewsInfovisModule.h
#	)
#ENDIF()
