target_link_libraries(${libname} PUBLIC iA::guibase)
target_link_libraries(${libname} PRIVATE
	iA::renderer iA::slicer
)
set(VTK_REQUIRED_LIBS_PRIVATE
	CommonComputationalGeometry # for vtkParametricSpline used in iAParametricSpline
)
if (openiA_CHART_OPENGL)
	TARGET_COMPILE_DEFINITIONS(${libname} PRIVATE CHART_OPENGL)
endif()
