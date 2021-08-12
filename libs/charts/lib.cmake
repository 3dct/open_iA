target_link_libraries(${libname} PUBLIC iA::qthelper)
if (MSVC AND MSVC_VERSION GREATER_EQUAL 1910)              # apparently required for VS < 2019:
	target_link_libraries(${libname} PUBLIC Opengl32)
endif()
set(VTK_REQUIRED_LIBS_PRIVATE
	ImagingStatistics       # for vtkImageAccumulate
)
if (openiA_CHART_OPENGL)
	TARGET_COMPILE_DEFINITIONS(${libname} PUBLIC CHART_OPENGL)
endif()
include(CMakeDependentOption)
cmake_dependent_option(openiA_CHART_SP_OLDOPENGL "You can enable this if you have an Nvidia graphics cards for quite some performance gain in scatter plot (matrix). Enabling it is known to cause problems on AMD graphics cards." OFF "openiA_CHART_OPENGL" ON)
if (openiA_CHART_OPENGL AND openiA_CHART_SP_OLDOPENGL)
	TARGET_COMPILE_DEFINITIONS(${libname} PRIVATE SP_OLDOPENGL)
endif()
if (openiA_OPENGL_DEBUG)
	TARGET_COMPILE_DEFINITIONS(${libname} PRIVATE OPENGL_DEBUG)
endif()
