set(DEPENDENCIES_LIBRARIES
	iA::charts      # also pulls in required iAqthelper
	iA::slicer      # for access to iASlicerImpl, also pulls in required iAguibase
)
set(DEPENDENCIES_VTK_MODULES
	ChartsCore  # for vtkAxis
	RenderingContext2D    # for vtkContextScene
	RenderingContextOpenGL2 # for implementation of RenderingContext2D (?)
	ViewsContext2D        # for vtkContextView
)
# for VTK < 9 we have to use VTK_USE_FILE anyway for module autoinitialization
#if (VTK_VERSION VERSION_LESS "9.0.0")
#	list(APPEND DEPENDENCIES_VTK_MODULES
#		ViewsCore     # for vtkViewsCoreModule.h, required by vtkViewsContext2DModule.h (iAScatterPlotView.cpp)
#	)
#endif()
set(DEPENDENCIES_IA_TOOLKIT_DIRS
	Entropy
)
set(DEPENDENCIES_MODULES
	MetaFilters   # for iASingleResult, iASamplingResults
)
