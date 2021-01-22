SET( DEPENDENCIES_LIBRARIES
	iAcore
	iArenderer         # for iARendererManager
)
SET( DEPENDENCIES_VTK_MODULES
	FiltersHybrid      # for vtkDepthSortPolyData
)
SET( DEPENDENCIES_ITK_MODULES
	ITKLabelMap        # for LabelObject, LabelMap
	ITKReview          # for LabelGeometryImageFilter
	ITKSmoothing       # for DiscreteGaussianImageFilter
	ITKThresholding    # for BinaryThresholdImageFilter
	ITKTransform       # for Transform
	ITKVtkGlue         # for ImageToVTKImageFilter
)
