SET( DEPENDENCIES_LIBRARIES
	iAbase
)
SET( DEPENDENCIES_VTK_MODULES
	FiltersGeometry    # for vtkDataSetSurfaceFilter used in iAExtractSurfaceFilter
	FiltersModeling    # for vtkFillHolesFilter
	IOGeometry         # for vtkSTLWriter
)
