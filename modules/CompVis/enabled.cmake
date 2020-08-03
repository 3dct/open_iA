if (VTK_MAJOR_VERSION GREATER 8)
	vtk_module_autoinit(TARGETS CompVis MODULES ${VTK_LIBRARIES})
endif()