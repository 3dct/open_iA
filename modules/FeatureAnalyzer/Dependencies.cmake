SET( DEPENDENCIES_LIBRARIES
	iAcharts	# also pulls in required iAqthelper
	iAobjectvis
	iArenderer  # for iARendererImpl and iARendererManager
	iAslicer    # also pulls in iAcore
	${VTK_LIB_PREFIX}ChartsCore            # for vtkPlot
	${VTK_LIB_PREFIX}RenderingContext2D    # for vtkContextScene
	${VTK_LIB_PREFIX}RenderingContext${VTK_RENDERING_BACKEND}    # for implementation of RenderingContext2D (?)
	${VTK_LIB_PREFIX}ViewsContext2D        # for vtkContextView
)
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	MaximumDistance
)
SET( DEPENDENCIES_MODULES
	FeatureAnalyzerComputation  # mainly for FeatureAnalyzerHelper
	FeatureScout
)
