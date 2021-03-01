TARGET_LINK_LIBRARIES(${libname} PUBLIC iAcore)
TARGET_LINK_LIBRARIES(${libname} PRIVATE
	iAcharts iAqthelper iArenderer iAslicer
)
SET( VTK_REQUIRED_LIBS_PRIVATE
	CommonComputationalGeometry # for vtkParametricSpline used in iAParametricSpline
)
if (openiA_CHART_OPENGL)
	TARGET_COMPILE_DEFINITIONS(${libname} PRIVATE CHART_OPENGL)
endif()
