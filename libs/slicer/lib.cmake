TARGET_LINK_LIBRARIES(${libname} PUBLIC iAcore)
TARGET_LINK_LIBRARIES(${libname} PRIVATE
	${VTK_LIB_PREFIX}CommonComputationalGeometry # for vtkParametricSpline used in iASpline
)
