if (NOT UNIX)
	option (openiA_ONNX_CUDA "Whether to use CUDA for running ONNX. Note that CUDA support is currently BROKEN in open_iA! If disabled, DirectML will be used (on Windows)." OFF)
else()
	set (openiA_ONNX_CUDA ON)
endif ()
if (UNIX OR openiA_ONNX_CUDA)
	target_compile_definitions(AI PRIVATE ONNX_CUDA)
endif()

find_library(DIRECTML_LIBRARY DirectML
	PATHS
		${ONNX_RUNTIME_DIR}/lib
		${ONNX_RUNTIME_DIR}/runtimes/win-x64/native
)

if (openiA_ONNX_CUDA)
	message(WARNING "ONNX CUDA support in open_iA is currently BROKEN! On Windows, you can use DirectML instead (by disabling AI_ONNX_CUDA).")
else()
	if (${DIRECTML_LIBRARY} STREQUAL "DIRECTML_LIBRARY-NOTFOUND")
		message(WARNING "You did not specify a DirectML library to use! If you build a release, the AI segmentation will not work. Download a DirectML runtime package from https://www.nuget.org/packages/Microsoft.AI.DirectML, extract it somewhere and specify the path to the DirectML.dll (typically in the bin/x64-win subfolder) in the DIRECTML_LIBRARY variable!")
	else()
		if (MSVC)
			install(FILES ${DIRECTML_LIBRARY} DESTINATION .)
			get_filename_component(DIRECTML_LIB_DIR "${DIRECTML_LIBRARY}" DIRECTORY)
			cmake_path(NATIVE_PATH DIRECTML_LIB_DIR DIRECTML_WIN_DIR)
			set(WinDLLPaths "${DIRECTML_WIN_DIR};${WinDLLPaths}")
		endif()
	endif()
endif()


# set ONNX_TYPE and ONNX_VERSION
if (ONNX_RUNTIME_INCLUDE_DIR MATCHES "/build/native/include$")
	set(ONNX_TYPE "DirectML")
	string(REGEX REPLACE "/build/native/include$" "" ONNX_BASE_PATH ${ONNX_RUNTIME_INCLUDE_DIR})
	set(ONNX_VERSION_FILE "${ONNX_BASE_PATH}/Microsoft.ML.OnnxRuntime.DirectML.nuspec")
	file(READ ${ONNX_VERSION_FILE} ONNX_NUGET_MANIFEST)
	string(REPLACE "\n" ";" ONNX_NUGET_LINES "${ONNX_NUGET_MANIFEST}")
	foreach(line ${ONNX_NUGET_LINES})
		if (line MATCHES "</version>$")
			string(REGEX REPLACE "    <version>(.+)</version>" "\\1" ONNX_VERSION "${line}")
			break()
		endif()
	endforeach()
	#message(STATUS "ONNX_BASE_PATH: ${ONNX_BASE_PATH}; ONNX_VERSION_FILE: ${ONNX_VERSION_FILE}; ONNX_VERSION: ${ONNX_VERSION}")
else()
	set(ONNX_TYPE "CUDA")
	string(REGEX REPLACE "/include$" "" ONNX_BASE_PATH ${ONNX_RUNTIME_INCLUDE_DIR})
	set(ONNX_VERSION_FILE "${ONNX_BASE_PATH}/VERSION_NUMBER")
	file(READ ${ONNX_VERSION_FILE} ONNX_VERSION)
	string(REPLACE "\n" "" ONNX_VERSION "${ONNX_VERSION}")
	string(REPLACE "\r" "" ONNX_VERSION "${ONNX_VERSION}")
	#message(STATUS "ONNX_BASE_PATH: ${ONNX_BASE_PATH}; ONNX_VERSION_FILE: ${ONNX_VERSION_FILE}; ONNX_VERSION: ${ONNX_VERSION}")
endif()
set (ONNX_MINIMUM_VERSION "1.13.1")
if (ONNX_VERSION VERSION_LESS ${ONNX_MINIMUM_VERSION})
	message(SEND_ERROR "ONNX runtime: Only versions >= ${ONNX_MINIMUM_VERSION} are supported, but encountered version ${ONNX_VERSION}!")
endif()


if (MSVC)
	get_filename_component(ONNX_LIB_DIR ${ONNX_RUNTIME_LIBRARIES} DIRECTORY)
	cmake_path(NATIVE_PATH ONNX_LIB_DIR ONNX_LIB_WIN_DIR)
	set(WinDLLPaths "${ONNX_LIB_WIN_DIR};${WinDLLPaths}" PARENT_SCOPE)
endif()

get_filename_component(ONNX_LIB_DIR ${ONNX_RUNTIME_LIBRARIES} DIRECTORY)
install(FILES ${ONNX_LIB_DIR}/onnxruntime.dll DESTINATION .)

set(BUILD_INFO "${BUILD_INFO}    \"ONNX runtime	${ONNX_VERSION} (GPU backend: ${ONNX_TYPE})\\n\"\n")
message(STATUS "ONNX runtime: ${ONNX_VERSION} (GPU backend: ${ONNX_TYPE})")
