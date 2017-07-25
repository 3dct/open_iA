# Modules which are used by the given module
SET( DEPENDENCIES_MODULES
)

# Cmake defines which are used by the given module
SET( DEPENDENCIES_CMAKE
	open_iA_GPU_USING_OPENCL
)

# Libraries which are used by the module
SET( DEPENDENCIES_LIBRARIES
	Dreamcaster
	${OPENCL_LIBRARIES}
)

# Toolkit directories
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
	Dreamcaster
)
