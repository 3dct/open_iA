target_link_libraries(${libname} PUBLIC iA::guibase)
target_link_libraries(${libname} PRIVATE
	iA::charts iA::qthelper iA::renderer iA::slicer iA::io
)
set(VTK_REQUIRED_LIBS_PRIVATE
	CommonComputationalGeometry # for vtkParametricSpline used in iAParametricSpline
)
if (openiA_CHART_OPENGL)
	TARGET_COMPILE_DEFINITIONS(${libname} PRIVATE CHART_OPENGL)
endif()
