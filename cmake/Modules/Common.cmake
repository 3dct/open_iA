#-------------------------
# Disable In-Source Build
#-------------------------
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
	message(FATAL_ERROR "In-source builds in ${CMAKE_BINARY_DIR} are disabled to avoid "
		"cluttering the source repository. Please delete ./CMakeCache.txt and ./CMakeFiles/, "
		"and run cmake with a newly created build directory.")
endif("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")

#-------------------------
# CTest
#-------------------------
option (openiA_TESTING_ENABLED "Whether to enable testing. This allows to run CTest/ CDash builds. Default: disabled." OFF)
IF (openiA_TESTING_ENABLED)
	INCLUDE (CTest)
	enable_testing()
	IF (openiA_USE_IDE_FOLDERS)
		SET_PROPERTY(TARGET Continuous PROPERTY FOLDER "_CTest")
		SET_PROPERTY(TARGET Experimental PROPERTY FOLDER "_CTest")
		SET_PROPERTY(TARGET Nightly PROPERTY FOLDER "_CTest")
		SET_PROPERTY(TARGET NightlyMemoryCheck PROPERTY FOLDER "_CTest")
	ENDIF()
ENDIF()

#-------------------------
# Output Directories
#-------------------------
IF(MSVC)
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/x64/Debug")
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/x64/Release")
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/x64/RelWithDebInfo")
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/x64/MinSizeRel")
ELSEIF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug")
	SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")
	SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo")
	SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel")
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug")
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo")
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel")
ELSE()
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
	SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
ENDIF()


#-------------------------
# LIBRARIES
#-------------------------

# ITK
SET(SAVED_CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}")
FIND_PACKAGE(ITK REQUIRED)
MESSAGE(STATUS "ITK: ${ITK_VERSION} in ${ITK_DIR}")
IF(ITK_VERSION_MAJOR LESS 4 OR (ITK_VERSION_MAJOR EQUAL 4 AND ITK_VERSION_MINOR LESS 10))
	MESSAGE(FATAL_ERROR "Your ITK version is too old. Please use ITK >= 4.10")
ENDIF()
SET (ITK_COMPONENTS
	ITKConvolution
	ITKDenoising
	ITKDistanceMap
	ITKGPUAnisotropicSmoothing
	ITKImageFeature
	ITKImageFusion
	ITKImageNoise
	ITKLabelMap
	ITKMesh
	ITKReview       # for LabelGeometryImageFilter
	ITKTestKernel   # for PipelineMonitorImageFilter
	ITKVtkGlue
	ITKWatersheds)
IF (ITK_VERSION_MAJOR GREATER 4 OR (ITK_VERSION_MAJOR EQUAL 4 AND ITK_VERSION_MINOR GREATER 12))
	LIST (APPEND ITK_COMPONENTS ITKImageIO)
	LIST (APPEND ITK_COMPONENTS ITKIORAW)  # apparently not included in ITKImageIO
ELSE()
	FOREACH( mod IN LISTS ITK_MODULES_ENABLED)
		IF( ${mod} MATCHES "IO")
			LIST (APPEND ITK_COMPONENTS ${mod})
		ENDIF()
	ENDFOREACH()
ENDIF()
# ITK has been found in sufficient version, otherwise above REQUIRED / FATAL_ERROR would have triggered CMake abort
# Now set it up with the components we need:
FIND_PACKAGE(ITK COMPONENTS ${ITK_COMPONENTS})
# apparently ITK (at least v5.0.0) adapts CMAKE_MODULE_PATH (bug?), reset it:
SET(CMAKE_MODULE_PATH "${SAVED_CMAKE_MODULE_PATH}")
INCLUDE(${ITK_USE_FILE})  # maybe avoid by using INCLUDE/LINK commands on targets instead?
IF (MSVC)
	SET (ITK_LIB_DIR "${ITK_DIR}/bin/Release")
ELSE()
	SET (ITK_LIB_DIR "${ITK_DIR}/lib")
ENDIF()
LIST (APPEND BUNDLE_DIRS "${ITK_LIB_DIR}")
IF (SCIFIO_LOADED)
	ADD_DEFINITIONS(-DUSE_SCIFIO)
	MESSAGE(STATUS "    SCIFIO support enabled!\n\
      Notice that in order to run a build with this library on another machine\n\
      than the one you built it, the environment variable SCIFIO_PATH\n\
      has to be set to the path containing the SCIFIO jar files!\n\
      Otherwise loading images will fail!")
	SET (SCIFIO_PATH "${ITK_DIR}/lib/jars")
	IF (MSVC)
		# variable will be set to the debugging environment instead of copying (see gui/CMakeLists.txt)
	ELSE(MSVC)
		SET (DESTDIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scifio_jars")
		MESSAGE(STATUS "Copying SCIFIO jars from ${SCIFIO_PATH} to ${DESTDIR}")
		configure_file("${SCIFIO_PATH}/bioformats_package.jar" "${DESTDIR}/bioformats_package.jar" COPYONLY)
		configure_file("${SCIFIO_PATH}/scifio-itk-bridge.jar" "${DESTDIR}/scifio-itk-bridge.jar" COPYONLY)
	ENDIF(MSVC)
	INSTALL(FILES "${SCIFIO_PATH}/bioformats_package.jar" DESTINATION scifio_jars)
	INSTALL(FILES "${SCIFIO_PATH}/scifio-itk-bridge.jar" DESTINATION scifio_jars)
ENDIF(SCIFIO_LOADED)


# VTK
FIND_PACKAGE(VTK REQUIRED)
MESSAGE(STATUS "VTK: ${VTK_VERSION} in ${VTK_DIR}")
MESSAGE(STATUS "    Rendering Backend: ${VTK_RENDERING_BACKEND}")
IF(VTK_VERSION_MAJOR LESS 8)
	MESSAGE(FATAL_ERROR "Your VTK version is too old. Please use VTK >= 8.0")
ENDIF()
SET (VTK_COMPONENTS
	vtkFiltersModeling         # for vtkRotationalExtrusionFilter, vtkOutlineFilter
	vtkInteractionWidgets      # for vtkScalarBarWidget/Representation
	vtkImagingStatistics       # for vtkImageAccumulate
	vtkIOGeometry              # for vtkSTLReader/Writer
	vtkIOMovie                 # for vtkGenericMovieWriter
	vtkRenderingAnnotation     # for vtkAnnotatedCubeActor, vtkScalarBarActor
	vtkRenderingImage          # for vtkImageResliceMapper
	vtkRenderingVolume${VTK_RENDERING_BACKEND}  # for volume rendering
	vtkRenderingQt             # for vtkQImageToImageSource, also pulls in vtkGUISupportQt (for QVTKWidgetOpenGL)
	vtkViewsContext2D          # for vtkContextView, vtkContextInteractorStyle
	vtkViewsInfovis)           # for vtkGraphItem
IF ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
	ADD_DEFINITIONS(-DVTK_OPENGL2_BACKEND)
ELSE()
	IF (MSVC)
		ADD_COMPILE_OPTIONS(/wd4081)
	ENDIF()
	LIST (APPEND VTK_COMPONENTS vtkGUISupportQtOpenGL)    # for QVTKWidget2
ENDIF()
IF (vtkRenderingOpenVR_LOADED)
	MESSAGE(STATUS "    RenderingOpenVR: available")
	LIST (APPEND VTK_COMPONENTS vtkRenderingOpenVR)
	STRING(FIND "${vtkRenderingOpenVR_INCLUDE_DIRS}" ";" semicolonpos REVERSE)
	math(EXPR aftersemicolon "${semicolonpos}+1")
	STRING(SUBSTRING "${vtkRenderingOpenVR_INCLUDE_DIRS}" ${aftersemicolon} -1 OPENVR_PATH_INCLUDE)
	STRING(REGEX REPLACE "/headers" "" OPENVR_PATH ${OPENVR_PATH_INCLUDE})
    IF (WIN32)
		SET (OPENVR_LIB_PATH "${OPENVR_PATH}/bin/win64")
	ELSE ()
		SET (OPENVR_LIB_PATH "${OPENVR_PATH}/bin/linux64")
	ENDIF()
	LIST (APPEND BUNDLE_DIRS "${OPENVR_LIB_PATH}")
ELSE()
	MESSAGE(STATUS "    RenderingOpenVR: NOT available! Enable Module_vtkRenderingOpenVR in VTK to make it available.")
ENDIF()
FIND_PACKAGE(VTK COMPONENTS ${VTK_COMPONENTS})
INCLUDE(${VTK_USE_FILE})  # maybe avoid by using INCLUDE/LINK commands on targets instead?
IF (MSVC)
	SET (VTK_LIB_DIR "${VTK_DIR}/bin/Release")
ELSE ()
	SET (VTK_LIB_DIR "${VTK_DIR}/lib")
ENDIF()
LIST (APPEND BUNDLE_DIRS "${VTK_LIB_DIR}")
IF (vtkoggtheora_LOADED OR vtkogg_LOADED)
	MESSAGE(STATUS "    Video: Ogg Theora Encoder available")
	ADD_DEFINITIONS(-DVTK_USE_OGGTHEORA_ENCODER)
ELSE()
	MESSAGE(WARNING "    Video: No encoder available! You will not be able to record videos.")
ENDIF()


# Qt (>= 5)
SET(CMAKE_AUTOMOC ON)
SET(QT_USE_QTXML TRUE)
#IF (WIN32)
#	SET( CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "C:/Program Files (x86)/Windows Kits/8.1/Lib/winv6.3/um/x64" )
#ENDIF (WIN32)
FIND_PACKAGE(Qt5 COMPONENTS Widgets Xml Network Test OpenGL PrintSupport REQUIRED)
MESSAGE(STATUS "Qt: ${Qt5_VERSION} in ${Qt5_DIR}")
IF (Qt5_VERSION_MINOR LESS 9)
	MESSAGE(FATAL_ERROR "Your Qt version is too old. Please use Qt >= 5.9")
ENDIF()
# Qt5OpenGL_INCLUDE_DIRS seems to be required on linux only, but doesn't hurt on Windows:
INCLUDE_DIRECTORIES(${Qt5Widgets_INCLUDE_DIRS} ${Qt5OpenGL_INCLUDE_DIRS} )
SET(QT_LIBRARIES ${Qt5Core_LIBRARIES} ${Qt5Xml_LIBRARIES} ${Qt5OpenGL_LIBRARIES} ${Qt5Network_LIBRARIES} ${Qt5PrintSupport_LIBRARIES})

STRING(REGEX REPLACE "/lib/cmake/Qt5" "" Qt5_BASEDIR ${Qt5_DIR})
STRING(REGEX REPLACE "/cmake/Qt5" "" Qt5_BASEDIR ${Qt5_BASEDIR})	# on linux, lib is omitted if installed from package repos
IF(WIN32)
	SET (QT_LIB_DIR "${Qt5_BASEDIR}/bin")
	# use imported targets for windows plugin:
	INSTALL (FILES "$<TARGET_FILE:Qt5::QWindowsIntegrationPlugin>" DESTINATION platforms)
ENDIF(WIN32)
IF (UNIX AND NOT APPLE AND NOT FLATPAK_BUILD)
	IF (EXISTS "${Qt5_BASEDIR}/lib")
		SET (QT_LIB_DIR "${Qt5_BASEDIR}/lib")
	ELSE()
		SET (QT_LIB_DIR "${Qt5_BASEDIR}")
	ENDIF()

	# xcb platform plugin, and its plugins egl and glx:
	# INSTALL (FILES "$<TARGET_FILE:Qt5::QXcbIntegrationPlugin>" DESTINATION platforms)
	# 
	IF (EXISTS ${Qt5_BASEDIR}/plugins)
		INSTALL (FILES ${Qt5_BASEDIR}/plugins/platforms/libqxcb.so DESTINATION platforms)
		INSTALL (DIRECTORY ${Qt5_BASEDIR}/plugins/xcbglintegrations DESTINATION .)
	ELSEIF (EXISTS ${Qt5_BASEDIR}/qt5/plugins)
		INSTALL (FILES ${Qt5_BASEDIR}/qt5/plugins/platforms/libqxcb.so DESTINATION platforms)
		INSTALL (DIRECTORY ${Qt5_BASEDIR}/qt5/plugins/xcbglintegrations DESTINATION .)
	ELSE()
		MESSAGE(SEND_ERROR "Qt Installation: xcb platform plugin (File libqxcb.so and directory xcbglintegrations) not found!")
	ENDIF()

	# install icu:
	# TODO: find out whether Qt was built with icu library dependencies
	# (typically only the case if webengine/webkit were included); but there
	# doesn't seem to be any CMake variable exposing this...
	SET(ICU_LIBS icudata icui18n icuuc)
	FOREACH(ICU_LIB ${ICU_LIBS})
		SET (ICU_LIB_LINK ${QT_LIB_DIR}/lib${ICU_LIB}.so)
		get_filename_component(ICU_SHAREDLIB "${ICU_LIB_LINK}" REALPATH)
		get_filename_component(ICU_SHAREDLIB_NAMEONLY "${ICU_SHAREDLIB}" NAME)
		STRING(LENGTH "${ICU_SHAREDLIB_NAMEONLY}" ICULIB_STRINGLEN)
		MATH(EXPR ICULIB_STRINGLEN "${ICULIB_STRINGLEN}-2")
		STRING(SUBSTRING "${ICU_SHAREDLIB_NAMEONLY}" 0 ${ICULIB_STRINGLEN} ICU_SHAREDLIB_DST)
		IF (EXISTS "${ICU_SHAREDLIB}")
			INSTALL (FILES "${ICU_SHAREDLIB}" DESTINATION . RENAME "${ICU_SHAREDLIB_DST}")
		ENDIF()
	ENDFOREACH()
ENDIF()
LIST (APPEND BUNDLE_DIRS "${QT_LIB_DIR}")


# Eigen
FIND_PACKAGE(Eigen3)
IF(EIGEN3_FOUND)
	ADD_DEFINITIONS(-DUSE_EIGEN)
	INCLUDE_DIRECTORIES( ${EIGEN3_INCLUDE_DIR} )
ENDIF(EIGEN3_FOUND)


# HDF5
# ToDo: Check for whether hdf5 is build as shared or static library,
# prefer static but also enable utilization of shared?
FIND_PACKAGE(HDF5 NAMES hdf5 COMPONENTS C NO_MODULE QUIET)

IF (HDF5_FOUND)
	if (WIN32)
		SET (HDF5_CORE_LIB_NAME libhdf5.lib)
		SET (HDF5_Z_LIB_NAME libzlib.lib)
		SET (HDF5_SZIP_LIB_NAME libszip.lib)
	else()
		SET (HDF5_CORE_LIB_NAME libhdf5.a)
		SET (HDF5_Z_LIB_NAME libz.a)
		SET (HDF5_SZIP_LIB_NAME libszip.a)
	endif()
	FIND_PATH(HDF5_INCLUDE_OVERWRITE_DIR hdf5.h PATHS "${HDF5_DIR}/../../include" "${HDF5_DIR}/../../../include")
	SET(HDF5_INCLUDE_DIR "${HDF5_INCLUDE_OVERWRITE_DIR}" CACHE PATH "" FORCE)
	UNSET(HDF5_INCLUDE_OVERWRITE_DIR CACHE)
	FIND_LIBRARY(HDF5_CORE_LIB ${HDF5_CORE_LIB_NAME} PATHS ${HDF5_DIR}/../../lib ${HDF5_DIR}/../../../lib)
	FIND_LIBRARY(HDF5_Z_LIB ${HDF5_Z_LIB_NAME} PATHS ${HDF5_DIR}/../../lib ${HDF5_DIR}/../../../lib NO_DEFAULT_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_PATH NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_FIND_ROOT_PATH)
	FIND_LIBRARY(HDF5_SZIP_LIB ${HDF5_SZIP_LIB_NAME} PATHS ${HDF5_DIR}/../../lib ${HDF5_DIR}/../../../lib)
	SET (HDF5_LIBRARY ${HDF5_CORE_LIB} ${HDF5_CORE_HL_LIB} ${HDF5_TOOL_LIB} ${HDF5_SZIP_LIB} ${HDF5_Z_LIB} CACHE STRING "" FORCE)
	UNSET(HDF5_Z_LIB CACHE)
	UNSET(HDF5_SZIP_LIB CACHE)
	UNSET(HDF5_CORE_LIB CACHE)
ENDIF()


# Astra Toolbox
FIND_PACKAGE(AstraToolbox)
IF (ASTRA_TOOLBOX_FOUND)
	IF (WIN32)
		SET (ASTRA_LIB_DIR "${ASTRA_TOOLBOX_DIR}/bin/x64/Release_CUDA")
	ELSEIF(UNIX AND NOT APPLE)
		get_filename_component(ASTRA_LIB_DIR "${ASTRA_TOOLBOX_LIBRARIES_RELEASE}" DIRECTORY)
	ENDIF ()
ENDIF()
LIST (APPEND BUNDLE_DIRS "${ASTRA_LIB_DIR}")


# CUDA:
FIND_PACKAGE(CUDA)
IF (CUDA_FOUND)
	ADD_DEFINITIONS(-DASTRA_CUDA)
	IF (WIN32)
		SET (CUDA_LIB_DIR ${CUDA_TOOLKIT_ROOT_DIR}/bin)
	ELSEIF(UNIX AND NOT APPLE)
		get_filename_component(CUDA_LIB_DIR "${CUDA_CUDART_LIBRARY}" DIRECTORY)
		get_filename_component(CUFFT_LIB_DIR "${CUDA_cufft_LIBRARY}" DIRECTORY)
		IF (NOT "${CUDA_LIB_DIR}" STREQUAL "${CUFFT_LIB_DIR}")
			MESSAGE(STATUS "CudaRT / CuFFT libs in different folders!")
			LIST (APPEND BUNDLE_DIRS "${CUFFT_LIB_DIR}")
		ENDIF()
	ENDIF ()
ENDIF()
LIST (APPEND BUNDLE_DIRS "${CUDA_LIB_DIR}")


# OpenCL
FIND_PACKAGE(OpenCL)
IF (OPENCL_FOUND)
	IF (WIN32)
		# OPENCL_LIBRARIES is set fixed to the OpenCL.lib file, but we need the dll
		# at least for AMD APP SDK, the dll is located in a similar location, just "bin" instead of "lib":
		STRING(REGEX REPLACE "lib/x86_64" "bin/x86_64" OPENCL_DLL "${OPENCL_LIBRARIES}")
		STRING(REGEX REPLACE "OpenCL.lib" "OpenCL.dll" OPENCL_DLL "${OPENCL_DLL}")
		IF (NOT EXISTS "${OPENCL_DLL}")
			SET (OPENCL_DLL "C:/Program Files/NVIDIA Corporation/OpenCL/OpenCL.dll") # installed along with NVidia driver
		ENDIF()
		IF (NOT EXISTS "${OPENCL_DLL}")
			MESSAGE(STATUS "OpenCL.dll was not found. You can continue building, but the program might not run (or it might fail to run when installed/cpacked).")
		ELSE()
			INSTALL (FILES ${OPENCL_DLL} DESTINATION .)
		ENDIF()
	ELSEIF (UNIX AND NOT APPLE)
		# typically OPENCL_LIBRARIES will only contain the one libOpenCL.so anyway, FOREACH just to make sure
		FOREACH(OPENCL_LIB ${OPENCL_LIBRARIES})
			get_filename_component(OPENCL_SHAREDLIB "${OPENCL_LIB}" REALPATH)
			INSTALL (FILES "${OPENCL_SHAREDLIB}" DESTINATION . RENAME libOpenCL.so.1)
		ENDFOREACH()
	ENDIF()
ENDIF()


# OpenMP
INCLUDE(${CMAKE_ROOT}/Modules/FindOpenMP.cmake)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")



#-------------------------
# Compiler Flags
#-------------------------
IF (WIN32)
	ADD_DEFINITIONS(-DCL_COMP) # TODO: check if that really is used for something!
ENDIF (WIN32)
IF (MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /EHsc")  # multi-processor compilation and common exception handling strategy
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
	ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS)
ENDIF (MSVC)
IF (CMAKE_COMPILER_IS_GNUCXX)
	IF (CMAKE_VERSION VERSION_LESS "3.8")
		MESSAGE(STATUS "Aiming for C++14 support.")
		SET(CMAKE_CXX_STANDARD 14)
	ELSEIF (CMAKE_VERSION VERSION_LESS "3.11")
		MESSAGE(STATUS "Aiming for C++17 support.")
		SET(CMAKE_CXX_STANDARD 17)
	ELSE()
		MESSAGE(STATUS "Aiming for C++20 support.")
		SET(CMAKE_CXX_STANDARD 20)
	ENDIF()
	SET(CMAKE_CXX_EXTENSIONS OFF)
	# Make sure at least C++ 0x is supported:
	INCLUDE (CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
	IF (NOT COMPILER_SUPPORTS_CXX0X)
		MESSAGE(WARN "The used compiler ${CMAKE_CXX_COMPILER} has no C++0x/11 support. Please use a newer C++ compiler.")
	ENDIF()

	execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpfullversion -dumpversion OUTPUT_VARIABLE GCC_VERSION)
	string(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
	list(GET GCC_VERSION_COMPONENTS 0 GCC_MAJOR)
	list(GET GCC_VERSION_COMPONENTS 1 GCC_MINOR)

	SET (CTEST_BUILD_COMMAND "make -i -j8")

	IF (GCC_MAJOR GREATER 3)
		IF (GCC_MINOR GREATER 2)
			set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -msse4.1 -fpermissive -lgomp -march=core2 -O2")
			set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -msse4.1 -lgomp -march=core2 -O2")
		ENDIF(GCC_MINOR GREATER 2)
	ELSE (GCC_MINOR GREATER 2)
		set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fpermissive -lgomp")
	ENDIF (GCC_MAJOR GREATER 3)

	IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		# Mac OS X specific code
		MESSAGE (STATUS "You are using MacOS - note that we do not regularly build on Mac OS, expect there to be some errors.")
	ENDIF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

ENDIF (CMAKE_COMPILER_IS_GNUCXX)


#-------------------------
# Common Settings
#-------------------------

SET (CORE_LIBRARY_NAME open_iA_Core)

option (openiA_USE_IDE_FOLDERS "Whether to group projects in subfolders in the IDE (mainly Visual Studio). Default: enabled." ON)
IF (openiA_USE_IDE_FOLDERS)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CMake")
ENDIF()

# open_iA Version number
include(GetGitRevisionDescription)
git_describe(openiA_VERSION --tags)
IF (WIN32)
	STRING(SUBSTRING "${CMAKE_SYSTEM_VERSION}" 0 3 WIN_VER)
	IF ("${WIN_VER}" STREQUAL "6.1")
		SET (openiA_VERSION "${openiA_VERSION}-w7")
	ENDIF()
ENDIF ()
MESSAGE(STATUS "Build version: ${openiA_VERSION}")
configure_file("${open_iA_SOURCE_DIR}/cmake/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)    # Enable Unicode

IF (UNIX)
    SET(CMAKE_INSTALL_RPATH "\$ORIGIN")      # Set RunPath in all created libraries / executables to $ORIGIN
    #IF (CMAKE_MAJOR_VERSION GREATER 3 OR (CMAKE_MAJOR_VERSION EQUAL 3 AND CMAKE_MINOR_VERSION GREATER 13))
    #    SET (CMAKE_BUILD_RPATH_USE_ORIGIN ON)
    #ENDIF ()
ENDIF ()
