set(DEPENDENCIES_LIBRARIES
	iA::guibase
	iA::slicer
)
set(DEPENDENCIES_VTK_MODULES
	ChartsCore            # for vtkAxis, vtkScatterPlotMatrix
	RenderingContextOpenGL2 # for implementation of RenderingContext2D (?)
	ViewsContext2D        # for vtkContextView, vtkContextInteractorStyle
)