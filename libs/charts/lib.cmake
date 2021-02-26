TARGET_LINK_LIBRARIES(${libname} PUBLIC
	iAqthelper
)
SET(VTK_REQUIRED_LIBS_PRIVATE
	ImagingStatistics       # for vtkImageAccumulate
)
if (openiA_CHART_OPENGL)
	TARGET_COMPILE_DEFINITIONS(${libname} PUBLIC CHART_OPENGL)
endif()
