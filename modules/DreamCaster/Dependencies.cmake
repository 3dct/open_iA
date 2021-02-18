SET( DEPENDENCIES_LIBRARIES
	OpenCL
	iAcharts
	iAcore
)
SET( DEPENDENCIES_VTK_MODULES
	FiltersHybrid    # for vtkDepthSortPolyData
)
IF (VTK_VERSION VERSION_LESS "9.0.0")
	LIST(APPEND DEPENDENCIES_VTK_MODULES
		sys     # for vtksys/SystemTools.hxx, required by iASTLLoader.cpp
	)
ENDIF()
