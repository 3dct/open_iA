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
# for VTK < 9 we have to use VTK_USE_FILE anyway for module autoinitialization
#if (VTK_VERSION VERSION_LESS "9.0.0")
#	LIST(APPEND DEPENDENCIES_VTK_MODULES
#		ViewsCore     # for vtkViewsCoreModule.h, required by vtkViewsContext2DModule.h (dlg_Consensus.cpp)
#	)
#endif()
#set(DEPENDENCIES_ITK_MODULES
#	ITKImageCompare           # for STAPLEImageFilter
#	ITKImageStatistics        # for LabelStatisticsImageFilter
#	ITKLabelVoting            # for MultiLabelSTAPLEImageFilter
#	ITKStatistics             # for Histogram
#)
set(DEPENDENCIES_IA_TOOLKIT_DIRS
	LabelVoting
)
set(DEPENDENCIES_MODULES
	MetaFilters   # for iASingleResult, iASamplingResults
)
