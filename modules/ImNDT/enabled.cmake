if (RenderingOpenVR IN_LIST VTK_COMPONENTS)
	set(BUILD_INFO "${BUILD_INFO}    \"OpenVR	${OpenVR_VERSION_MAJOR}.${OpenVR_VERSION_MINOR}.${OpenVR_VERSION_PATCH}\\n\"\n")
endif()

if (RenderingOpenXR IN_LIST VTK_COMPONENTS)
	set(BUILD_INFO "${BUILD_INFO}    \"OpenXR	${OpenXR_VERSION}\\n\"\n")
endif()

set(VR_INSTALL_SRC_SUBDIRS images)
set(VR_INSTALL_DST_SUBDIRS VR-skybox)
list(APPEND VR_INSTALL_SRC_SUBDIRS action_manifests)
list(APPEND VR_INSTALL_DST_SUBDIRS VR-input-manifests)
list(LENGTH VR_INSTALL_SRC_SUBDIRS VR_INSTALL_DIRS_COUNT)
math(EXPR VR_INSTALL_DIRS_COUNT "${VR_INSTALL_DIRS_COUNT} - 1") # subtract one since foreach loop includes end index

foreach (VR_INSTALL_DIR_IDX RANGE ${VR_INSTALL_DIRS_COUNT})
	list(GET VR_INSTALL_SRC_SUBDIRS ${VR_INSTALL_DIR_IDX} VR_SRC_SUBDIR)
	list(GET VR_INSTALL_DST_SUBDIRS ${VR_INSTALL_DIR_IDX} VR_DST_SUBDIR)
	set(VR_INSTALL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ImNDT/${VR_SRC_SUBDIR}/")
	foreach (cfg ${CMAKE_CONFIGURATION_TYPES})
		string(TOUPPER "${cfg}" CFG)
		set(VR_INSTALL_DST_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG}}/${VR_DST_SUBDIR}")
		if (NOT EXISTS "${VR_INSTALL_DST_DIR}")
			file(COPY "${VR_INSTALL_SRC_DIR}" DESTINATION "${VR_INSTALL_DST_DIR}")
		endif()
	endforeach()
	install(DIRECTORY "${VR_INSTALL_SRC_DIR}" DESTINATION ${VR_DST_SUBDIR})
endforeach()
