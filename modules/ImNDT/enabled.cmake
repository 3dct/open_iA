if (RenderingOpenVR IN_LIST VTK_COMPONENTS)
	set(BUILD_INFO "${BUILD_INFO}    \"OpenVR	${OpenVR_VERSION_MAJOR}.${OpenVR_VERSION_MINOR}.${OpenVR_VERSION_PATCH}\\n\"\n")
endif()

if (RenderingOpenXR IN_LIST VTK_COMPONENTS)
	set(BUILD_INFO "${BUILD_INFO}    \"OpenXR	${OpenXR_VERSION}\\n\"\n")
endif()

set(VTK_VERSION_MAJMIN "${VTK_VERSION_MAJOR}.${VTK_VERSION_MINOR}")
set(VR_INSTALL_SRC_SUBDIRS images "action_manifests/vtk${VTK_VERSION_MAJMIN}")
set(VR_INSTALL_DST_SUBDIRS VR-skybox VR-input-manifests)
list(LENGTH VR_INSTALL_SRC_SUBDIRS VR_INSTALL_DIRS_COUNT)
math(EXPR VR_INSTALL_DIRS_COUNT "${VR_INSTALL_DIRS_COUNT} - 1") # subtract one since foreach loop includes end index

foreach (VR_INSTALL_DIR_IDX RANGE ${VR_INSTALL_DIRS_COUNT})
	list(GET VR_INSTALL_SRC_SUBDIRS ${VR_INSTALL_DIR_IDX} VR_SRC_SUBDIR)
	list(GET VR_INSTALL_DST_SUBDIRS ${VR_INSTALL_DIR_IDX} VR_DST_SUBDIR)
	set(VR_INSTALL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ImNDT/${VR_SRC_SUBDIR}/")
	set(targetname ImNDT_copy-${VR_DST_SUBDIR})
	add_custom_target(${targetname} COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
			"${VR_INSTALL_SRC_DIR}" "$<TARGET_FILE_DIR:iA::guibase>/${VR_DST_SUBDIR}" COMMENT "Copying files")
	add_dependencies(ImNDT ${targetname})
	if (openiA_USE_IDE_FOLDERS)
		SET_PROPERTY(TARGET ${targetname} PROPERTY FOLDER "_CMake")
	endif()
	install(DIRECTORY "${VR_INSTALL_SRC_DIR}" DESTINATION ${VR_DST_SUBDIR})
endforeach()
