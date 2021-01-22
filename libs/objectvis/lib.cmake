TARGET_LINK_LIBRARIES(${libname} PUBLIC	iAbase)
SET( VTK_REQUIRED_LIBS_PRIVATE
	FiltersModeling         # for vtkOutlineFilter
)
