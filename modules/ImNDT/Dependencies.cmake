if (NOT RenderingOpenVR IN_LIST VTK_AVAILABLE_COMPONENTS)
	message(WARNING "OpenVR not available in VTK 9! Please enable RenderingOpenVR module!")
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
