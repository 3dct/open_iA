set(DEPENDENCIES_LIBRARIES
	iA::charts	          # also pulls in required iA::qthelper
	iA::objectvis
	iA::renderer          # for iARendererImpl and iARendererViewSync
	iA::slicer            # also pulls in iA::guibase
)
set(DEPENDENCIES_VTK_MODULES
	ChartsCore            # for vtkPlot
	RenderingContext2D    # for vtkContextScene
	RenderingContextOpenGL2 # for implementation of RenderingContext2D
	ViewsContext2D        # for vtkContextView
)
set(DEPENDENCIES_IA_TOOLKIT_DIRS
	MaximumDistance
)
set(DEPENDENCIES_MODULES
	FeatureAnalyzerComputation  # mainly for FeatureAnalyzerHelper
	FeatureScout
)
