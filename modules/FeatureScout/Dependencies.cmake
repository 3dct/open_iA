SET (DEPENDENCIES_LIBRARIES
	charts
	objectvis
)

get_filename_component(ObjectVisSrcDir "../core/objectvis" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
SET( DEPENDENCIES_INCLUDE_DIRS
	${ObjectVisSrcDir}
)