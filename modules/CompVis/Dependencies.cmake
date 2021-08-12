set(DEPENDENCIES_LIBRARIES
	iA::guibase
)
set(DEPENDENCIES_VTK_MODULES
	ChartsCore            # for vtkAxis
	FiltersHybrid         # for vtkPolyDataSilhouette
	ImagingHybrid         # for vtkSampleFunction used in iABlobCluster
	RenderingContext2D    # for vtkContextActor, vtkContextScene, vtkContextTransform
	RenderingContextOpenGL2 # for implementation of RenderingContext2D (?)
	ViewsContext2D        # for vtkContextView, vtkContextInteractorStyle
	InfovisCore
	InfovisLayout
	ViewsInfovis
	FiltersStatistics
	FiltersExtraction
)