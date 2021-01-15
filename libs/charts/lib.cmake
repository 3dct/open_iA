TARGET_LINK_LIBRARIES(${libname} PUBLIC iAqthelper)
TARGET_LINK_LIBRARIES(${libname} PRIVATE
	${VTK_LIB_PREFIX}ImagingStatistics       # for vtkImageAccumulate
)
if (openiA_CHART_OPENGL)
	TARGET_COMPILE_DEFINITIONS(${libname} PUBLIC CHART_OPENGL)
endif()
