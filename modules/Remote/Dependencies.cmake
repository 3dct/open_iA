if (NOT Qt${QT_VERSION_MAJOR}WebSockets_DIR)
	set(Qt${QT_VERSION_MAJOR}WebSockets_DIR "${Qt${QT_VERSION_MAJOR}_DIR}WebSockets" CACHE PATH "" FORCE)
endif()
find_package(Qt${QT_VERSION_MAJOR}WebSockets REQUIRED)

set(DEPENDENCIES_LIBRARIES
	iA::guibase
	Qt${QT_VERSION_MAJOR}::WebSockets
)

#set(VTK_REQUIRED_LIBS_PUBLIC
#	IOImage               # for image writing
#	RenderingCore
#)