SET( DEPENDENCIES_LIBRARIES
	iAbase
	${VTK_LIB_PREFIX}FiltersGeometry    # for vtkDataSetSurfaceFilter used in iAExtractSurfaceFilter
	${VTK_LIB_PREFIX}FiltersModeling    # for vtkFillHolesFilter
	${VTK_LIB_PREFIX}IOGeometry         # for vtkSTLWriter
)
