SET( DEPENDENCIES_LIBRARIES
	iAguibase
	iArenderer         # for iARendererViewSync
)
SET( DEPENDENCIES_VTK_MODULES
	FiltersHybrid      # for vtkDepthSortPolyData
)
#SET( DEPENDENCIES_ITK_MODULES
#	ITKLabelMap        # for LabelObject, LabelMap
#	ITKReview          # for LabelGeometryImageFilter
#	ITKSmoothing       # for DiscreteGaussianImageFilter
#	ITKThresholding    # for BinaryThresholdImageFilter
#	ITKTransform       # for Transform
#	ITKVtkGlue         # for ImageToVTKImageFilter
#)
