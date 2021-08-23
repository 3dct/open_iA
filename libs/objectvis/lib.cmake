target_link_libraries(${libname} PUBLIC	iA::base)
set(VTK_REQUIRED_LIBS_PRIVATE
	FiltersModeling         # for vtkOutlineFilter
)
# for VTK < 9 we have to use VTK_USE_FILE anyway for module autoinitialization
#IF (VTK_VERSION VERSION_LESS "9.0.0")
#	LIST(APPEND VTK_REQUIRED_LIBS_PRIVATE
#		CommonTransforms       # for vtkTransform.h, required by vtkEllipsoidSource.cpp
#		FiltersGeneral         # for vtkFiltersGeneralModule.h, required by vtkFiltersModelingModule.h
#	)
#ENDIF()
