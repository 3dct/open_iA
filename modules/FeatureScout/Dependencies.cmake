SET( DEPENDENCIES_LIBRARIES
	iAcharts      # also pulls in required iAqthelper
	iAobjectvis
	iAguibase
)
SET( DEPENDENCIES_VTK_MODULES
	ChartsCore            # for vtkAxis
	FiltersHybrid         # for vtkPolyDataSilhouette
	ImagingHybrid         # for vtkSampleFunction used in iABlobCluster
	RenderingContext2D    # for vtkContextActor, vtkContextScene, vtkContextTransform
	RenderingContext${VTK_RENDERING_BACKEND}    # for implementation of RenderingContext2D (?)
	ViewsContext2D        # for vtkContextView, vtkContextInteractorStyle
)
# for VTK < 9 we have to use VTK_USE_FILE anyway for module autoinitialization
#IF (VTK_VERSION VERSION_LESS "9.0.0")
#	LIST(APPEND DEPENDENCIES_VTK_MODULES
#		ViewsCore     # for vtkViewsCoreModule.h, required by vtkViewsContext2DModule.h (dlg_FeatureScout)
#	)
#ENDIF()
#SET( DEPENDENCIES_ITK_MODULES
#	ITKLabelMap                            # for LabelImageToLabelMapFilter, LabelMapMaskImageFilter
#	ITKSmoothing                           # for DiscreteGaussianImageFilter
#	ITKThresholding                        # for BinaryThresholdImageFilter
#	ITKVtkGlue                             # for ImageToVTKImageFilter / VTKImageToImageFilter
#)
