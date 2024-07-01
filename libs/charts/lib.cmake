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
set(COLORMAP_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/charts/colormaps/")
add_custom_target(iAcharts_copy-colormaps COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
    "${COLORMAP_SRC_DIR}" "$<TARGET_FILE_DIR:${libname}>/colormaps"
    COMMENT "Copying colormaps...")
add_dependencies(${libname} iAcharts_copy-colormaps)
if (openiA_USE_IDE_FOLDERS)
	SET_PROPERTY(TARGET iAcharts_copy-colormaps PROPERTY FOLDER "_CMake")
endif()
install(DIRECTORY "${COLORMAP_SRC_DIR}" DESTINATION colormaps)
