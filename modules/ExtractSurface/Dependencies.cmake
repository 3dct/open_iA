set(DEPENDENCIES_LIBRARIES
	iA::base
)
set(DEPENDENCIES_VTK_MODULES
	FiltersGeometry    # for vtkDataSetSurfaceFilter used in iAExtractSurfaceFilter
	FiltersModeling    # for vtkFillHolesFilter
	IOGeometry         # for vtkSTLWriter
)
