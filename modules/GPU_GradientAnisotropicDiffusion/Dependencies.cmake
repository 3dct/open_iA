# Modules which are used by the given module
SET( DEPENDENCIES_MODULES
)

# Cmake defines which are used by the given module
SET( DEPENDENCIES_CMAKE
	open_iA_GPU_USING_OPENCL
)

# Include directories used by the module
SET( DEPENDENCIES_INCLUDE_DIRS
	${OPENCL_INCLUDE_DIRS}
)

# Libraries which are used by the module
SET( DEPENDENCIES_LIBRARIES
	${OPENCL_LIBRARIES}
)
