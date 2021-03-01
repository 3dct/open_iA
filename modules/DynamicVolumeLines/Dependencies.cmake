SET( DEPENDENCIES_LIBRARIES
	iAcharts    # for qcustomplot, also pulls in required iAqthelper
	iAcore
)
#SET( DEPENDENCIES_ITK_MODULES
#	ITKVtkGlue       # for ImageToVTKImageFilter
#	ITKStatistics    # for ImageToHistogramFilter
#)
SET( DEPENDENCIES_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/DynamicVolumeLines/CompactHilbert/include
)
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	FunctionalBoxplot
)
