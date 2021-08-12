set(DEPENDENCIES_LIBRARIES
	iA::charts    # for qcustomplot, also pulls in required iAqthelper
	iA::guibase
)
#set(DEPENDENCIES_ITK_MODULES
#	ITKVtkGlue       # for ImageToVTKImageFilter
#	ITKStatistics    # for ImageToHistogramFilter
#)
set(DEPENDENCIES_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/DynamicVolumeLines/CompactHilbert/include
)
set(DEPENDENCIES_IA_TOOLKIT_DIRS
	FunctionalBoxplot
)
