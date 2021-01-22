SET( DEPENDENCIES_LIBRARIES
	iAcharts      # also pulls in required iAqthelper
	iAobjectvis
	iAcore
)
SET( DEPENDENCIES_VTK_MODULES
	ChartsCore            # for vtkAxis
	FiltersHybrid         # for vtkPolyDataSilhouette
	ImagingHybrid         # for vtkSampleFunction used in iABlobCluster
	RenderingContext2D    # for vtkContextActor, vtkContextScene, vtkContextTransform
	RenderingContext${VTK_RENDERING_BACKEND}    # for implementation of RenderingContext2D (?)
	ViewsContext2D        # for vtkContextView, vtkContextInteractorStyle
)
SET( DEPENDENCIES_ITK_MODULES
	ITKLabelMap                            # for LabelImageToLabelMapFilter, LabelMapMaskImageFilter
	ITKSmoothing                           # for DiscreteGaussianImageFilter
	ITKThresholding                        # for BinaryThresholdImageFilter
	ITKVtkGlue                             # for ImageToVTKImageFilter / VTKImageToImageFilter
)