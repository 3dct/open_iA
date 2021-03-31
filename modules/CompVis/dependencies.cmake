SET( DEPENDENCIES_LIBRARIES
	iAguibase
)
SET( DEPENDENCIES_VTK_MODULES
	ChartsCore            # for vtkAxis
	FiltersHybrid         # for vtkPolyDataSilhouette
	ImagingHybrid         # for vtkSampleFunction used in iABlobCluster
	RenderingContext2D    # for vtkContextActor, vtkContextScene, vtkContextTransform
	RenderingContext${VTK_RENDERING_BACKEND}    # for implementation of RenderingContext2D (?)
	ViewsContext2D        # for vtkContextView, vtkContextInteractorStyle
	InfovisCore
	InfovisLayout
	ViewsInfovis
	FiltersStatistics
	FiltersExtraction
)
