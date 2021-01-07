SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	Entropy
)
SET( DEPENDENCIES_LIBRARIES
	iAcharts      # implicitly imported by iAgui
	# iAqthelper    # pulled in by iAcharts automatically
)
SET (DEPENDENCIES_MODULES
	MetaFilters
)

SET (DEPENDENCIES_BASE_LIBRARY iAslicer) # for access to iASlicerImpl