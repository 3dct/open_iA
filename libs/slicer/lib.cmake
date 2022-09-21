target_link_libraries(${libname} PUBLIC iA::guibase)
set(VTK_REQUIRED_LIBS_PRIVATE
	CommonComputationalGeometry # for vtkParametricSpline used in iASpline
)
