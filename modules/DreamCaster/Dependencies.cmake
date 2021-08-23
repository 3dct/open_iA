set(DEPENDENCIES_LIBRARIES
	OpenCL
	iA::charts
	iA::guibase
)
set(DEPENDENCIES_VTK_MODULES
	FiltersHybrid    # for vtkDepthSortPolyData
)
# for VTK < 9 we have to use VTK_USE_FILE anyway for module autoinitialization
#if (VTK_VERSION VERSION_LESS "9.0.0")
#	list(APPEND DEPENDENCIES_VTK_MODULES
#		sys     # for vtksys/SystemTools.hxx, required by iASTLLoader.cpp
#	)
#endif()
