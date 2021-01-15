TARGET_LINK_LIBRARIES(${libname} PUBLIC iAbase)
TARGET_LINK_LIBRARIES(${libname} PRIVATE
	${VTK_LIB_PREFIX}FiltersModeling         # for vtkOutlineFilter
)
