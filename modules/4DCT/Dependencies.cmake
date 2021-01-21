SET (DEPENDENCIES_LIBRARIES
	iAcore
	iArenderer                        # for iARendererManager
	${VTK_LIB_PREFIX}FiltersHybrid    # for vtkDepthSortPolyData
)
SET (DEPENDENCIES_ITK_MODULES
	ITKTransform                      # for Transform
	ITKVtkGlue                        # for ImageToVTKImageFilter
)
