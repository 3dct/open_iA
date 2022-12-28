option (openiA_ONNX_CUDA "Whether to use CUDA for running ONNX. If disabled, DirectML will be used (on Windows)." OFF)
option (openiA_ONNX_NEWNAMEFUNCTIONS "Set to true for ONNX >= 1.13.1; with that version, the GetOutputNames and GetInputNames functions have been renamed to GetOutputNamesAllocated and GetInputNamesAllocated." OFF)
if (UNIX OR openiA_ONNX_CUDA)
	target_compile_definitions(AI PRIVATE ONNX_CUDA)
endif()

if (openiA_ONNX_NEWNAMEFUNCTIONS)
	target_compile_definitions(AI PRIVATE ONNX_NEWNAMEFUNCTIONS)
endif()