target_link_libraries(${libname} PUBLIC	iA::base)
set(VTK_REQUIRED_LIBS_PRIVATE
	FiltersModeling         # for vtkOutlineFilter
)
