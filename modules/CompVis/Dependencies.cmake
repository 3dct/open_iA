set(DEPENDENCIES_LIBRARIES
	iA::guibase
	iA::objectvis
	iA::renderer
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
	FiltersProgrammable
	CommonComputationalGeometry
	
)
IF (EIGEN3_FOUND)
	SET( DEPENDENCIES_INCLUDE_DIRS
		${EIGEN3_INCLUDE_DIR}
	)
ENDIF()

