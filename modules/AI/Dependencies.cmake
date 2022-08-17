set(ONNX_RUNTIME_DIR
	"${ONNX_RUNTIME_DIR}"
	CACHE
	PATH
	"Path to search for the ONNX Runtime"
)
find_library(ONNX_RUNTIME_LIBRARIES onnxruntime
	PATHS
		${ONNX_RUNTIME_DIR}/lib
		${ONNX_RUNTIME_DIR}/runtimes/win-x64/native
)
find_path(ONNX_RUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h
	PATHS
		${ONNX_RUNTIME_DIR}/include
		${ONNX_RUNTIME_DIR}/build/native/include
)
set(DEPENDENCIES_LIBRARIES
	${ONNX_RUNTIME_LIBRARIES}
	iA::base
)
set(DEPENDENCIES_INCLUDE_DIRS
	${ONNX_RUNTIME_INCLUDE_DIR}
)
