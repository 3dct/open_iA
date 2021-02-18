SET( DEPENDENCIES_LIBRARIES
	iAbase
)
SET( DEPENDENCIES_VTK_MODULES
	FiltersGeometry    # for vtkDataSetSurfaceFilter used in iAExtractSurfaceFilter
	FiltersModeling    # for vtkFillHolesFilter
	IOGeometry         # for vtkSTLWriter
)
IF (VTK_VERSION VERSION_LESS "9.0.0")
	LIST(APPEND DEPENDENCIES_VTK_MODULES
		CommonMisc          # for vtkContourValues.h, required by vtkFlyingEdges3D
		FiltersGeneral      # for vtkFiltersGeneralModule.h, required by vtkFiltersModelingModule.h
		IOCore              # for vtkWriter.h, required by vtkSTLWriter
	)
ENDIF()
