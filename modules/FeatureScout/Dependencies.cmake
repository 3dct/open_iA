set(DEPENDENCIES_LIBRARIES
	iA::charts      # also pulls in required iAqthelper
	iA::objectvis
	iA::guibase
)
set(DEPENDENCIES_VTK_MODULES
	ChartsCore            # for vtkAxis
	FiltersHybrid         # for vtkPolyDataSilhouette
	ImagingHybrid         # for vtkSampleFunction used in iABlobCluster
	RenderingContext2D    # for vtkContextActor, vtkContextScene, vtkContextTransform
	RenderingContextOpenGL2 # for implementation of RenderingContext2D (?)
	ViewsContext2D        # for vtkContextView, vtkContextInteractorStyle
)
# for VTK < 9 we have to use VTK_USE_FILE anyway for module autoinitialization
#if (VTK_VERSION VERSION_LESS "9.0.0")
#	list(APPEND DEPENDENCIES_VTK_MODULES
#		ViewsCore     # for vtkViewsCoreModule.h, required by vtkViewsContext2DModule.h (dlg_FeatureScout)
#	)
#endif()
#set(DEPENDENCIES_ITK_MODULES
#	ITKLabelMap                            # for LabelImageToLabelMapFilter, LabelMapMaskImageFilter
#	ITKSmoothing                           # for DiscreteGaussianImageFilter
#	ITKThresholding                        # for BinaryThresholdImageFilter
#	ITKVtkGlue                             # for ImageToVTKImageFilter / VTKImageToImageFilter
#)
