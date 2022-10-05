if (NOT Qt${QT_VERSION_MAJOR}WebSockets_DIR)
	set(Qt${QT_VERSION_MAJOR}WebSockets_DIR "${Qt${QT_VERSION_MAJOR}_DIR}WebSockets" CACHE PATH "" FORCE)
endif()
if (NOT Qt${QT_VERSION_MAJOR}HttpServer_DIR)
	set(Qt${QT_VERSION_MAJOR}HttpServer_DIR "${Qt${QT_VERSION_MAJOR}_DIR}HttpServer" CACHE PATH "" FORCE)
endif()
find_package(Qt${QT_VERSION_MAJOR}WebSockets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR}HttpServer)

set(DEPENDENCIES_LIBRARIES
	iA::guibase
	Qt${QT_VERSION_MAJOR}::WebSockets
)
if (Qt${QT_VERSION_MAJOR}HttpServer_FOUND)
	list(APPEND DEPENDENCIES_LIBRARIES Qt${QT_VERSION_MAJOR}::HttpServer)
	set(Qt${QT_VERSION_MAJOR}HttpServer_FOUND 1 PARENT_SCOPE) # required to have it available in enabled.cmake!
endif()

#set(VTK_REQUIRED_LIBS_PUBLIC
#	IOImage               # for image writing
#	RenderingCore
#)