SET ( DEPENDENCIES_INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/DynamicVolumeLines/CompactHilbert/include
)
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	FunctionalBoxplot
)
SET( DEPENDENCIES_LIBRARIES
	iAcharts    # for qcustomplot
	# iAqthelper    # pulled in by charts automatically
)