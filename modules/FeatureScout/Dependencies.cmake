set(DEPENDENCIES_LIBRARIES
	iA::charts
	iA::objectvis
	iA::guibase
)
set(DEPENDENCIES_VTK_MODULES
	ChartsCore            # for vtkAxis
	FiltersHybrid         # for vtkDepthSortPolyData, vtkPolyDataSilhouette
	ImagingHybrid         # for vtkSampleFunction used in iABlobCluster
	RenderingContext2D    # for vtkContextActor, vtkContextScene, vtkContextTransform
	RenderingContextOpenGL2 # for implementation of RenderingContext2D (?)
	ViewsContext2D        # for vtkContextView, vtkContextInteractorStyle
)
#set(DEPENDENCIES_ITK_MODULES
#	ITKLabelMap                            # for LabelImageToLabelMapFilter, LabelMapMaskImageFilter
#	ITKSmoothing                           # for DiscreteGaussianImageFilter
#	ITKThresholding                        # for BinaryThresholdImageFilter
#	ITKVtkGlue                             # for ImageToVTKImageFilter / VTKImageToImageFilter
#)
