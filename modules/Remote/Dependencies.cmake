if (NOT Qt6WebSockets_DIR)
	set(Qt6WebSockets_DIR "${Qt6_DIR}WebSockets" CACHE PATH "" FORCE)
endif()
if (NOT Qt6HttpServer_DIR)
	set(Qt6HttpServer_DIR "${Qt6_DIR}HttpServer" CACHE PATH "" FORCE)
endif()
find_package(Qt6WebSockets REQUIRED)
find_package(Qt6HttpServer)

set(DEPENDENCIES_LIBRARIES
	iA::guibase
	iA::slicer
	Qt::WebSockets
)

set(DEPENDENCIES_MODULES
	Labelling
)

if (Qt6HttpServer_FOUND)
	list(APPEND DEPENDENCIES_LIBRARIES Qt::HttpServer)
	set(Qt6HttpServer_FOUND 1 PARENT_SCOPE) # required to have it available in enabled.cmake!
endif()

if (CUDAToolkit_FOUND)
	list(APPEND DEPENDENCIES_LIBRARIES CUDA::cudart CUDA::nvjpeg CUDA::nppc CUDA::nppig)
endif()

set(DEPENDENCIES_VTK_MODULES
#	IOImage               # for image writing
	RenderingImage        # for vtkImageResliceMapper
)