option (openiA_ONNX_CUDA "Whether to use CUDA for running ONNX. If disabled, DirectML will be used (on Windows)." OFF)
option (openiA_ONNX_CUDA_NEW "Whether to use the 'new' way of initializing CUDA; set to ON when using ONNX runtime  >= 1.8.x or 1.9.x (otherwise you'll receive a compile error relating to 'cuda_provider_factory.h')." OFF)
if (UNIX OR openiA_ONNX_CUDA)
	target_compile_definitions(AI PRIVATE ONNX_CUDA)
endif()

if (openiA_ONNX_CUDA_NEW)
	target_compile_definitions(ONNX_CUDA_NEW)
endif()