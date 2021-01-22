SET( DEPENDENCIES_LIBRARIES
	iAcore
	iAqthelper
)
SET( DEPENDENCIES_ITK_MODULES
	ITKConnectedComponents    # for RelabelComponentImageFilter
	ITKImageLabel                # for ScanlineFilterCommon, dependency of ConnectedComponentImageFilter
)
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	Entropy
)
