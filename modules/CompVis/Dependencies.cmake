set(DEPENDENCIES_LIBRARIES
	iA::guibase
	iA::objectvis
	iA::renderer
)
set(DEPENDENCIES_VTK_MODULES
	ChartsCore            # for vtkAxis
	CommonColor           # for vtkNamedColors, vtkColorSeries
	RenderingContext2D    # for vtkContextActor, vtkContextScene, vtkContextTransform
	RenderingContextOpenGL2 # for implementation of RenderingContext2D (?)
	ViewsContext2D        # for vtkContextView, vtkContextInteractorStyle
	InfovisLayout         # for vtkGraphLayoutStrategy
	ViewsInfovis          # for vtkGraphLayoutView
	FiltersStatistics     # for vtkComputeQuartiles, vtkCorrelativeStatistics
	FiltersExtraction     # for vtkExtractSelection
	FiltersProgrammable   # for vtkProgrammableGlyphFilter	
)
set(DEPENDENCIES_INCLUDE_DIRS
	${EIGEN3_INCLUDE_DIR}
)
set(DEPENDENCIES_CMAKE
	EIGEN3_FOUND
)
