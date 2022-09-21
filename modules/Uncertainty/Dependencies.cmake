set(DEPENDENCIES_LIBRARIES
	iA::charts      # also pulls in required iA::qthelper
	iA::slicer      # for access to iASlicerImpl, also pulls in required iA::guibase
)
set(DEPENDENCIES_VTK_MODULES
	ChartsCore  # for vtkAxis
	RenderingContext2D    # for vtkContextScene
	RenderingContextOpenGL2 # for implementation of RenderingContext2D (?)
	ViewsContext2D        # for vtkContextView
)
set(DEPENDENCIES_IA_TOOLKIT_DIRS
	Entropy
)
set(DEPENDENCIES_MODULES
	MetaFilters   # for iASingleResult, iASamplingResults
)
