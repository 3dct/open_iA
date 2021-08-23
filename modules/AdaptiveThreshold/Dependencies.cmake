if (NOT Qt${QT_VERSION_MAJOR}Charts_DIR)
	set(Qt${QT_VERSION_MAJOR}Charts_DIR "${Qt${QT_VERSION_MAJOR}_DIR}Charts" CACHE PATH "" FORCE)
endif()
find_package(Qt${QT_VERSION_MAJOR}Charts REQUIRED)

set(DEPENDENCIES_LIBRARIES
	Qt${QT_VERSION_MAJOR}::Charts
	iA::charts
	iA::guibase
)
set(DEPENDENCIES_MODULES
	Segmentation
)
