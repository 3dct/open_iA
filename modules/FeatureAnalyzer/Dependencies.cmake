SET (DEPENDENCIES_IA_TOOLKIT_DIRS
	MaximumDistance
)

SET (DEPENDENCIES_MODULES
	FeatureScout
)

get_filename_component(ObjectVisSrcDir "../core/objectvis" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
get_filename_component(ObjectVisBinDir "../core/objectvis" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
SET( DEPENDENCIES_INCLUDE_DIRS "${ObjectVisSrcDir}" "${ObjectVisBinDir}")