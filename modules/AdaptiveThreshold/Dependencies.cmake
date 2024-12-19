if (NOT Qt6Charts_DIR)
	set(Qt6Charts_DIR "${Qt6_DIR}Charts" CACHE PATH "" FORCE)
endif()
find_package(Qt6Charts REQUIRED)

set(DEPENDENCIES_LIBRARIES
	Qt6::Charts
	iA::charts
	iA::guibase
)
set(DEPENDENCIES_MODULES
	Segmentation
)
