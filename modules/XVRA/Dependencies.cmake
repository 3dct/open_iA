set(DEPENDENCIES_LIBRARIES
	iA::guibase
	iA::objectvis
)
set(DEPENDENCIES_MODULES
	FeatureScout           
	VR
)

# Check whether boost (from astra) has histogram.hpp (only available in boost >= 1.70)
if (NOT BOOST_INCLUDE_DIR OR NOT EXISTS "${BOOST_INCLUDE_DIR}/boost/histogram.hpp")
	message(STATUS "Boost with histogram.hpp not found (specify via BOOST_INCLUDE_DIR)!")
	set(DEPENDENCIES_CMAKE BOOST_HISTOGRAM_HPP_FOUND)
endif()

set(DEPENDENCIES_INCLUDE_DIRS
	${BOOST_INCLUDE_DIR}
)