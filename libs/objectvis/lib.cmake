TARGET_LINK_LIBRARIES(${libname} PUBLIC	iAbase)
SET( VTK_REQUIRED_LIBS_PRIVATE
	FiltersModeling         # for vtkOutlineFilter
)
IF (VTK_VERSION VERSION_LESS "9.0.0")
	LIST(APPEND VTK_REQUIRED_LIBS_PRIVATE
		CommonTransforms       # for vtkTransform.h, required by vtkEllipsoidSource.cpp
		FiltersGeneral         # for vtkFiltersGeneralModule.h, required by vtkFiltersModelingModule.h
	)
ENDIF()
