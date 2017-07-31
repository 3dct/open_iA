# OpenCL:

IF (open_iA_GPU_USING_OPENCL)

	IF (${ITKGPUCommon_LIBRARY_DIRS} STREQUAL "")
		MESSAGE(SEND_ERROR "You're trying to build with OpenCL support, but your ITK build has OpenCL/GPU support disabled! Please build ITK libraries with ITK_USE_GPU enabled!")
	ENDIF()

	FIND_PACKAGE(OpenCL REQUIRED)
	IF(OPENCL_FOUND)
		INCLUDE_DIRECTORIES( ${OPENCL_INCLUDE_DIRS} )
	ENDIF()

	IF (WIN32)
		# OPENCL_LIBRARIES is set fixed to the OpenCL.lib file, but we need the dll
		# at least for AMD APP SDK, the dll is located in a similar location, just "bin" instead of "lib":
		STRING(REGEX REPLACE "lib/x86_64/OpenCL.lib" "bin/x86_64/OpenCL.dll" OPENCL_LIB ${OPENCL_LIBRARIES})
		INSTALL (FILES ${OPENCL_LIB} DESTINATION .)
	ENDIF (WIN32)
	IF (UNIX)
		# typically OPENCL_LIBRARIES will only contain the one libOpenCL.so anyway, FOREACH just to make sure
		# hard-coded .1 might have to be replaced at some point...
		FOREACH(OPENCL_LIB ${OPENCL_LIBRARIES})
			INSTALL (FILES ${OPENCL_LIB}.1 DESTINATION .)
		ENDFOREACH()
	ENDIF(UNIX)
ENDIF()

