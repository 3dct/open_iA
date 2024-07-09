# Check whether boost (from astra) has histogram.hpp (only available in boost >= 1.70)
if (NOT BOOST_INCLUDE_DIR OR NOT EXISTS "${BOOST_INCLUDE_DIR}/boost/histogram.hpp")
	message(STATUS "Boost with histogram.hpp not found (specify via BOOST_INCLUDE_DIR)!")
	set(DEPENDENCIES_CMAKE BOOST_HISTOGRAM_HPP_FOUND)
endif()

set(DEPENDENCIES_LIBRARIES
	iA::guibase
	iA::objectvis
)
set(DEPENDENCIES_INCLUDE_DIRS
	${BOOST_INCLUDE_DIR}
)
if (RenderingOpenVR IN_LIST VTK_COMPONENTS)
	list(APPEND DEPENDENCIES_COMPILE_DEFINITIONS
		OPENVR_VERSION_MAJOR=${OpenVR_VERSION_MAJOR} OPENVR_VERSION_MINOR=${OpenVR_VERSION_MINOR} OPENVR_VERSION_BUILD=${OpenVR_VERSION_PATCH}
		OPENVR_AVAILABLE)
	# list(APPEND DEPENDENCIES_LIBRARIES ${OpenVR_LIBRARY}) # added by RenderingOpenVR
	list(APPEND DEPENDENCIES_VTK_MODULES RenderingOpenVR)
	# list(APPEND DEPENDENCIES_INCLUDE_DIRS ${OpenVR_INCLUDE_DIR}) # added by RenderingOpenVR
endif()
if (RenderingOpenXR IN_LIST VTK_COMPONENTS)
	list(APPEND DEPENDENCIES_COMPILE_DEFINITIONS OPENXR_AVAILABLE)
	# list(APPEND DEPENDENCIES_LIBRARIES ${OpenXR_LIBRARY}) # added by RenderingOpenXR
	list(APPEND DEPENDENCIES_VTK_MODULES RenderingOpenXR)
	if (VTK_VERSION VERSION_LESS_EQUAL "9.3.1")
		list(APPEND DEPENDENCIES_INCLUDE_DIRS ${OpenXR_INCLUDE_DIR}/openxr) # not sure why this is NOT added (properly) by VTK's RenderingOpenXR module
	endif()
	# failing to add this causes compilation error in vtkOpenXR.h for `#include <openxr.h>`; maybe VTK forgets to export the OpenXR include directory?
endif()

if (NOT RenderingOpenXR IN_LIST VTK_COMPONENTS AND NOT RenderingOpenVR IN_LIST VTK_COMPONENTS)
	message(WARNING "Neither OpenVR nor OpenXR available in VTK! Please enable one of these two modules!")
	set(DEPENDENCIES_CMAKE RenderingOpenVR RenderingOpenXR)
endif()
