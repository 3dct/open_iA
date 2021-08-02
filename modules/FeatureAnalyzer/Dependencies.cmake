SET( DEPENDENCIES_LIBRARIES
	iAcharts	          # also pulls in required iAqthelper
	iAobjectvis
	iArenderer            # for iARendererImpl and iARendererViewSync
	iAslicer              # also pulls in iAguibase
)
SET( DEPENDENCIES_VTK_MODULES
	ChartsCore            # for vtkPlot
	RenderingContext2D    # for vtkContextScene
	RenderingContextOpenGL2 # for implementation of RenderingContext2D
	ViewsContext2D        # for vtkContextView
)
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	MaximumDistance
)
SET( DEPENDENCIES_MODULES
	FeatureAnalyzerComputation  # mainly for FeatureAnalyzerHelper
	FeatureScout
)
