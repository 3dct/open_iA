set(DEPENDENCIES_LIBRARIES
	iA::base
)
set(DEPENDENCIES_VTK_MODULES
	FiltersGeometry    # for vtkDataSetSurfaceFilter used in iAExtractSurfaceFilter
	FiltersModeling    # for vtkFillHolesFilter
	IOGeometry         # for vtkSTLWriter
)
# for VTK < 9 we have to use VTK_USE_FILE anyway for module autoinitialization
#if (VTK_VERSION VERSION_LESS "9.0.0")
#	list(APPEND DEPENDENCIES_VTK_MODULES
#		CommonMisc          # for vtkContourValues.h, required by vtkFlyingEdges3D
#		FiltersGeneral      # for vtkFiltersGeneralModule.h, required by vtkFiltersModelingModule.h
#		IOCore              # for vtkWriter.h, required by vtkSTLWriter
#	)
#endif()
