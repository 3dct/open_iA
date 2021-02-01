TARGET_LINK_LIBRARIES(${libname} PUBLIC iAcharts iAqthelper iArenderer iAslicer)
if (openiA_CHART_OPENGL)
	TARGET_COMPILE_DEFINITIONS(${libname} PUBLIC CHART_OPENGL)
endif()
