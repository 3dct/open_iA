SET( DEPENDENCIES_LIBRARIES
	iAcharts      # also pulls in required iAqthelper
	iAobjectvis
	iAcore
	${VTK_LIB_PREFIX}ChartsCore            # for vtkAxis
	${VTK_LIB_PREFIX}FiltersHybrid         # for vtkPolyDataSilhouette
	${VTK_LIB_PREFIX}ImagingHybrid         # for vtkSampleFunction used in iABlobCluster
	${VTK_LIB_PREFIX}RenderingContext2D    # for vtkContextActor, vtkContextScene, vtkContextTransform
	${VTK_LIB_PREFIX}RenderingContext${VTK_RENDERING_BACKEND}    # for implementation of RenderingContext2D (?)
	${VTK_LIB_PREFIX}ViewsContext2D        # for vtkContextView, vtkContextInteractorStyle
)
