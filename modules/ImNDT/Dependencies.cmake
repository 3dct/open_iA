if (RenderingOpenXR IN_LIST VTK_COMPONENTS)
	list(APPEND VR_BACKEND_OPTIONS OpenXR)
	set(VR_BACKEND_DEFAULT OpenXR)    # default: initialize to OpenXR...
endif()
if (RenderingOpenVR IN_LIST VTK_COMPONENTS)
	list(APPEND VR_BACKEND_OPTIONS OpenVR)
	set(VR_BACKEND_DEFAULT OpenVR)    # ... but set to OpenVR (for the moment) if available
endif()
if (NOT VR_BACKEND_OPTIONS)
	message(WARNING "Neither OpenVR nor OpenXR available in VTK! Please enable one of these two modules!")
endif()

list(FIND VR_BACKEND_OPTIONS "${openiA_VR_BACKEND}" VR_options_index)
if (${VR_options_index} EQUAL -1)
	if (DEFINED ${openiA_VR_BACKEND})
		message(WARNING "Invalid ${openiA_VR_BACKEND}, resetting to default ${VR_BACKEND_DEFAULT}!")
	endif()
	set(openiA_VR_BACKEND "${VR_BACKEND_DEFAULT}" CACHE STRING "Library to use for VR. OpenXR is more recent, but OpenVR support is currently better tested." FORCE)
endif()
set_property(CACHE openiA_VR_BACKEND PROPERTY STRINGS ${VR_BACKEND_OPTIONS})

# Check whether boost (from astra) has histogram.hpp (only available in boost >= 1.70)
if (NOT BOOST_INCLUDE_DIR OR NOT EXISTS "${BOOST_INCLUDE_DIR}/boost/histogram.hpp")
	message(STATUS "Boost with histogram.hpp not found (specify via BOOST_INCLUDE_DIR)!")
	set(DEPENDENCIES_CMAKE BOOST_HISTOGRAM_HPP_FOUND)
endif()

set(DEPENDENCIES_LIBRARIES
	iA::guibase
	iA::objectvis
	iA::qthelper
)
set(DEPENDENCIES_INCLUDE_DIRS
	${BOOST_INCLUDE_DIR}
)
if ("${openiA_VR_BACKEND}" STREQUAL "OpenVR")
	set(DEPENDENCIES_COMPILE_DEFINITIONS
		OPENVR_VERSION_MAJOR=${OpenVR_VERSION_MAJOR} OPENVR_VERSION_MINOR=${OpenVR_VERSION_MINOR} OPENVR_VERSION_BUILD=${OpenVR_VERSION_PATCH}
		OPENVR_AVAILABLE)
	# list(APPEND DEPENDENCIES_LIBRARIES ${OpenVR_LIBRARY}) # added by RenderingOpenVR
	list(APPEND DEPENDENCIES_VTK_MODULES RenderingOpenVR)
	# list(APPEND DEPENDENCIES_INCLUDE_DIRS ${OpenVR_INCLUDE_DIR}) # added by RenderingOpenVR
endif()

if ("${openiA_VR_BACKEND}" STREQUAL "OpenXR")
	set(DEPENDENCIES_COMPILE_DEFINITIONS OPENXR_AVAILABLE)
	# list(APPEND DEPENDENCIES_LIBRARIES ${OpenXR_LIBRARY}) # added by RenderingOpenXR
	list(APPEND DEPENDENCIES_VTK_MODULES RenderingOpenXR)
	list(APPEND DEPENDENCIES_INCLUDE_DIRS ${OpenXR_INCLUDE_DIR}/openxr) # not sure why this is NOT added (properly) by VTK's RenderingOpenXR module
	# failing to add this causes compilation error in vtkOpenXR.h for `#include <openxr.h>`; maybe VTK forgets to export the OpenXR include directory?
endif()
