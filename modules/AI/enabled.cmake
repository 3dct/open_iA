option (openiA_ONNX_CUDA "Whether to use CUDA for running ONNX. If disabled, DirectML will be used (on Windows)." OFF)
if (UNIX OR openiA_ONNX_CUDA)
	TARGET_COMPILE_DEFINITIONS(AI PRIVATE ONNX_CUDA)
endif()
