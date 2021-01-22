TARGET_LINK_LIBRARIES(${libname} PUBLIC iAcore)
SET( VTK_REQUIRED_LIBS_PRIVATE
	CommonComputationalGeometry # for vtkParametricSpline used in iASpline
)
