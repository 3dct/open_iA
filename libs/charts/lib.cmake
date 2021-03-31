TARGET_LINK_LIBRARIES(${libname} PUBLIC
	iAqthelper
)
IF (MSVC AND MSVC_VERSION GREATER_EQUAL 1910)              # apparently required for VS < 2019:
	TARGET_LINK_LIBRARIES(${libname} PUBLIC Opengl32)
ENDIF ()
SET(VTK_REQUIRED_LIBS_PRIVATE
	ImagingStatistics       # for vtkImageAccumulate
)
if (openiA_CHART_OPENGL)
	TARGET_COMPILE_DEFINITIONS(${libname} PUBLIC CHART_OPENGL)
endif()
