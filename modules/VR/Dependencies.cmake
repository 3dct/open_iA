if ( (VTK_MAJOR_VERSION LESS 9 AND vtkRenderingOpenVR_LOADED) OR
	 (VTK_MAJOR_VERSION GREATER 8 AND RenderingOpenVR IN_LIST VTK_AVAILABLE_COMPONENTS) )
	if (VTK_MAJOR_VERSION LESS 9)
		find_package(OpenVR)
		# HINTS ${OPENVR_PATH} # leads to OpenVR not being found at all...?
	endif()
else()
	message(STATUS "OpenVR SDK: Not found.")
endif()

if (VTK_MAJOR_VERSION LESS 9)
	set(DEPENDENCIES_CMAKE
		vtkRenderingOpenVR_LOADED
	)
else()
	if (NOT RenderingOpenVR IN_LIST VTK_AVAILABLE_COMPONENTS)
		message(WARNING "OpenVR not available in VTK 9! Please enable RenderingOpenVR module!")
	endif()
endif()

# Check whether boost (from astra) has histogram.hpp (only available in boost >= 1.70)
if (NOT BOOST_INCLUDE_DIR OR NOT EXISTS "${BOOST_INCLUDE_DIR}/boost/histogram.hpp")
	message(STATUS "Boost with histogram.hpp not found (specify via BOOST_INCLUDE_DIR)!")
	set(DEPENDENCIES_CMAKE BOOST_HISTOGRAM_HPP_FOUND)
endif()

set(DEPENDENCIES_LIBRARIES
	${OPENVR_LIBRARY}
	iA::guibase
	iA::objectvis
	iA::qthelper
)
set(DEPENDENCIES_INCLUDE_DIRS
	${OPENVR_INCLUDE_DIR}
	${BOOST_INCLUDE_DIR}
)
set(DEPENDENCIES_COMPILE_DEFINITIONS
	OPENVR_VERSION_MAJOR=${OPENVR_VERSION_MAJOR} OPENVR_VERSION_MINOR=${OPENVR_VERSION_MINOR} OPENVR_VERSION_BUILD=${OPENVR_VERSION_BUILD}
)
