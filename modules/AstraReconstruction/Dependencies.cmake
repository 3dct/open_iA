# Modules which are used by the given module
SET( DEPENDENCIES_MODULES
)

# Cmake defines which are used by the given module
SET( DEPENDENCIES_CMAKE
	ASTRA_TOOLBOX_FOUND
)

# Libraries which are used by the module
SET( DEPENDENCIES_LIBRARIES_DEBUG
	${ASTRA_TOOLBOX_LIBRARIES_DEBUG}
)

# Libraries which are used by the module
SET( DEPENDENCIES_LIBRARIES_RELEASE
	${ASTRA_TOOLBOX_LIBRARIES_RELEASE}
)

# Include directories used by the module
SET( DEPENDENCIES_INCLUDE_DIRS
	${ASTRA_TOOLBOX_INCLUDE_DIRS}
)

# iAnalyseToolkit directories
SET( DEPENDENCIES_IA_TOOLKIT_DIRS
)
