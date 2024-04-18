target_link_libraries(${libname} PUBLIC iA::base)
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

add_custom_command(TARGET ${libname} POST_BUILD
	COMMAND ${CMAKE_COMMAND} "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/charts/colormaps/" "-DTARGET_DIR=$<TARGET_FILE_DIR:${libname}>/colormaps" -P "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/copy-if-newer.cmake")

install(DIRECTORY "${COLORMAP_SRC_DIR}" DESTINATION colormaps)