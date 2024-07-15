# CMake Policies
# no settings required - minimum version 3.14 will set all policies as desired!

#-------------------------
# Disable In-Source Build
#-------------------------
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
if ("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
	message(FATAL_ERROR "In-source builds in ${CMAKE_BINARY_DIR} are disabled to avoid "
		"cluttering the source repository. Please delete ./CMakeCache.txt and ./CMakeFiles/, "
		"and run cmake with a newly created build directory.")
endif()

message(STATUS "CMake: ${CMAKE_VERSION}")

#-------------------------
# CTest
#-------------------------
option(openiA_TESTING_ENABLED "Whether to enable testing. This allows to run CTest/ CDash builds. Default: disabled." OFF)
if (openiA_TESTING_ENABLED)

	set(TEST_DATA_DIR "${TEST_DIR}/data")
	set(TEST_IMG_DIR "${CMAKE_BINARY_DIR}/Testing/Temporary")

	function(add_image_test)
		set(oneValueArgs NAME)
		set(multiValueArgs COMMAND)
		cmake_parse_arguments(TEST "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
		if (DEFINED TEST_UNPARSED_ARGUMENTS)
			message(SEND_ERROR "add_image_test called with unknown arguments '${TEST_UNPARSED_ARGUMENTS}'. Only NAME and COMMAND arguments are known!")
		endif()
		set(TEST_TEMPLATE_SCRIPT_FILE "${TEST_DIR}/run_test_with_img_output.cmake.in")
		set(TEST_OUTPUT_FILE ${TEST_IMG_DIR}/${TEST_NAME}.png)
		set(TEST_SCRIPT_NAME "${CMAKE_BINARY_DIR}/Test${TEST_NAME}.cmake")
		configure_file(${TEST_TEMPLATE_SCRIPT_FILE} ${TEST_SCRIPT_NAME} @ONLY)
		add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_COMMAND} -P ${TEST_SCRIPT_NAME})
	endfunction()

	function(add_openfile_test)
		set(oneValueArgs NAME FILENAME)
		cmake_parse_arguments(TEST "" "${oneValueArgs}" "" ${ARGN} )
		if (DEFINED TEST_UNPARSED_ARGUMENTS)
			message(SEND_ERROR "add_openfile_test called with unknown arguments '${TEST_UNPARSED_ARGUMENTS}'. Only NAME and FILENAME arguments are known!")
		endif()
		add_image_test(NAME ${TEST_NAME} COMMAND ${TEST_GUI_EXECUTABLE} ${TEST_DATA_DIR}/${TEST_FILENAME} --quit 500 --screenshot ${TEST_IMG_DIR}/${TEST_NAME}.png --size 1920x1080)
	endfunction()

	message(STATUS "Testing enabled")
	include (CTest)
	enable_testing()
	if (openiA_USE_IDE_FOLDERS)
		SET_PROPERTY(TARGET Continuous PROPERTY FOLDER "_CTest")
		SET_PROPERTY(TARGET Experimental PROPERTY FOLDER "_CTest")
		SET_PROPERTY(TARGET Nightly PROPERTY FOLDER "_CTest")
		SET_PROPERTY(TARGET NightlyMemoryCheck PROPERTY FOLDER "_CTest")
	endif()
endif()

option(CMAKE_UNITY_BUILD "Enable unity build" OFF)

#-------------------------
# Precompiled headers
#-------------------------
if (CMAKE_VERSION VERSION_GREATER "3.15.99")
	option(openiA_PRECOMPILE  "Whether to use precompiled headers to speed up build. Default: disabled." OFF)
	if (openiA_PRECOMPILE)
		message(STATUS "openiA_PRECOMPILE enabled.")
	endif()
endif()

option(openiA_CHART_OPENGL "Whether to use OpenGL in chart widgets" ON)
option(openiA_OPENGL_DEBUG "Enable this to turn on debugging messages in OpenGL windows (currently charts)." OFF)

#------------------------------
# Build / Compiler information
#------------------------------
set(BUILD_INFO "\"CMake	${CMAKE_VERSION} (Generator: ${CMAKE_GENERATOR})\\n\"\n")
if (MSVC)
	if (MSVC_VERSION LESS 1800)
		message(WARNING "Your Visual Studio version is too old and not supported! Please update to a newer version (>= Visual Studio 2013 (12.0)), or you will experience problems during build!")
	endif()
	message(STATUS "Compiler: Visual C++ (MSVC_VERSION ${MSVC_VERSION} / ${CMAKE_CXX_COMPILER_VERSION} / ${CMAKE_VS_VERSION_BUILD_NUMBER})")
	set(BUILD_INFO "${BUILD_INFO}    \"Compiler	Visual C++ (MSVC_VERSION ${MSVC_VERSION} / ${CMAKE_CXX_COMPILER_VERSION} / ${CMAKE_VS_VERSION_BUILD_NUMBER})\\n\"\n")
	set(BUILD_INFO "${BUILD_INFO}    \"Windows SDK	${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}\\n\"\n")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	message(STATUS "Compiler: Clang (${CMAKE_CXX_COMPILER_VERSION})")
	set(BUILD_INFO "${BUILD_INFO}    \"Compiler	Clang (Version ${CMAKE_CXX_COMPILER_VERSION})\\n\"\n")
elseif (CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "Compiler: G++ (${CMAKE_CXX_COMPILER_VERSION})")
	set(BUILD_INFO "${BUILD_INFO}    \"Compiler	G++ (Version ${CMAKE_CXX_COMPILER_VERSION})\\n\"\n")
else()
	message(WARNING "Unknown compiler! Please report any CMake or compilation errors on https://github.com/3dct/open_iA!")
	set(BUILD_INFO "${BUILD_INFO}    \"Compiler	Unknown\\n\"\n")
endif()
set(BUILD_INFO "${BUILD_INFO}    \"Target	${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}\\n\"\n")
if (FLATPAK_BUILD)
	set(BUILD_INFO "${BUILD_INFO}    \"Build Type	Flatpak\\n\"\n")
endif()

#-------------------------
# Output Directories
#-------------------------
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (${is_multi_config})
	message(STATUS "Multi-configuration generator")
	foreach (config ${CMAKE_CONFIGURATION_TYPES})
		string(TOUPPER "${config}" CONFIG)
		# On windows, both executable and dll's are considered RUNTIME, as well as executables on Mac OS...
		set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG} "${CMAKE_BINARY_DIR}/x64/${CONFIG}")
		if (APPLE)	# but .dylib's on Mac OS are considered LIBRARY (XCode generator - multi-config)
			set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG} "${CMAKE_BINARY_DIR}/x64/${CONFIG}")
		endif()
	endforeach()
else()
	message(STATUS "Single-configuration generator")
	# Set a default build type if none was specified
	if (NOT CMAKE_BUILD_TYPE)
		set(DEFAULT_BUILD_TYPE "Release")
		message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
		set(CMAKE_BUILD_TYPE ${DEFAULT_BUILD_TYPE} CACHE STRING "Choose the type of build." FORCE)
		# Set the possible values of build type for cmake-gui
		set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
	endif()
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()


#-------------------------
# LIBRARIES
#-------------------------

# Prepare empty BUNDLE directory:
set(BUNDLE_DIRS "")

# Suppress CMake warnings
#    - triggered for OpenMP package
#    - searched for within ITK and VTK
#    - see https://cmake.org/cmake/help/v3.17/module/FindPackageHandleStandardArgs.html
set(FPHSA_NAME_MISMATCHED 1)

# ITK
set(SAVED_CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}")
find_package(ITK REQUIRED)
message(STATUS "ITK: ${ITK_VERSION} in ${ITK_DIR}")
if (ITK_VERSION VERSION_LESS "5.0.0")
	message(FATAL_ERROR "Your ITK version is too old. Please use ITK >= 5.0")
endif()
set(ITK_COMPONENTS
	ITKConvolution
	ITKDenoising
	ITKDistanceMap
	ITKGPUAnisotropicSmoothing
	ITKImageFeature
	ITKImageFusion
	ITKImageIO
	ITKImageNoise
	ITKIORAW        # apparently not included in ITKImageIO
	ITKLabelMap
	ITKMesh
	ITKReview       # for RobustAutomaticThresholdImageFilter (Segmentation)
	ITKTestKernel   # for PipelineMonitorImageFilter
	ITKVtkGlue
	ITKWatersheds)
set(ITK_HGrad_INFO "off")
if (HigherOrderAccurateGradient_LOADED)
	message(STATUS "    HigherOrderAccurateGradient (HOAG) available as ITK module")
	set(ITK_HGrad_INFO "on")
	list(APPEND ITK_COMPONENTS HigherOrderAccurateGradient)
endif()
set(ITK_RTK_INFO "off")
if (RTK_LOADED)
	message(STATUS "    RTK ${RTK_VERSION_MAJOR}.${RTK_VERSION_MINOR}.${RTK_VERSION_PATCH} available as ITK module")
	set(ITK_RTK_INFO "${RTK_VERSION_MAJOR}.${RTK_VERSION_MINOR}.${RTK_VERSION_PATCH}")
	list(APPEND ITK_COMPONENTS RTK)
endif()
# ITK has been found in sufficient version, otherwise above REQUIRED / FATAL_ERROR would have triggered CMake abort
# Now set it up with the components we need:
find_package(ITK COMPONENTS ${ITK_COMPONENTS})
# apparently ITK (at least v5.0.0) adapts CMAKE_MODULE_PATH (bug?), reset it:
set(CMAKE_MODULE_PATH "${SAVED_CMAKE_MODULE_PATH}")
include(${ITK_USE_FILE}) # <- maybe avoid by using include/link commands on targets instead?
# -> ITK build system would have to be updated for that first, 5.x does not support it!
# problem: also does some factory initialization (IO), which cannot easily be called separately
set(ITK_BASE_DIR "${ITK_DIR}")
if (MSVC)
	set(ITK_LIB_DIR "${ITK_DIR}/bin/Release")
else()
	if (EXISTS "${ITK_DIR}/lib")
		set(ITK_LIB_DIR "${ITK_DIR}/lib")
	else()
		set(ITK_BASE_DIR "${ITK_DIR}/../../..")
		set(ITK_LIB_DIR "${ITK_BASE_DIR}/lib")
	endif()
endif()
list(APPEND BUNDLE_DIRS "${ITK_LIB_DIR}")
set(ITK_SCIFIO_INFO "off")
if (SCIFIO_LOADED)
	set(ITK_SCIFIO_INFO "on")
	message(STATUS "    SCIFIO support enabled!\n\
       Notice that in order to run a build with this library on another machine\n\
       than the one you built it, the environment variable SCIFIO_PATH\n\
       has to be set to the path containing the SCIFIO jar files!\n\
       Otherwise loading images will fail!")
	set(SCIFIO_PATH "${ITK_BASE_DIR}/lib/jars")
	if (MSVC)
		# variable will be set to the debugging environment instead of copying (see gui/CMakeLists.txt)
	else()
		set(DESTDIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scifio_jars")
		message(STATUS "Copying SCIFIO jars from ${SCIFIO_PATH} to ${DESTDIR}")
		configure_file("${SCIFIO_PATH}/bioformats_package.jar" "${DESTDIR}/bioformats_package.jar" COPYONLY)
		configure_file("${SCIFIO_PATH}/scifio-itk-bridge.jar" "${DESTDIR}/scifio-itk-bridge.jar" COPYONLY)
	endif()
	install(FILES "${SCIFIO_PATH}/bioformats_package.jar" DESTINATION scifio_jars)
	install(FILES "${SCIFIO_PATH}/scifio-itk-bridge.jar" DESTINATION scifio_jars)
endif()

set(ITK_GPU_INFO "on")
if (${ITK_USE_GPU} STREQUAL "OFF")
	set(ITK_GPU_INFO "off")
	if (NOT APPLE)  # ITK_USE_GPU not working anyway under Mac OS (see https://github.com/InsightSoftwareConsortium/ITK/issues/3821)
		message(WARNING "ITK is built without GPU support (flag ITK_USE_GPU disabled). "
			"Some GPU-optimized functionality (e.g. GPU-based anisotropic filter) will not be available!")
	endif()
else()
	message(STATUS "    GPU-accelerated filters (ITK_USE_GPU) enabled")
endif()
set(BUILD_INFO "${BUILD_INFO}    \"ITK	${ITK_VERSION} (GPU: ${ITK_GPU_INFO}, SCIFIO: ${ITK_SCIFIO_INFO}, RTK: ${ITK_RTK_INFO}, HOAG: ${ITK_HGrad_INFO})\\n\"\n")

# VTK
find_package(VTK REQUIRED)
message(STATUS "VTK: ${VTK_VERSION} in ${VTK_DIR}")
if (VTK_VERSION VERSION_LESS "9.1.0")
	message(FATAL_ERROR "Your VTK version is too old. Please use VTK >= 9.1")
endif()
set(VTK_LIB_PREFIX "VTK::")
# List of ALL VTK components we might need (for all modules)
set(VTK_COMPONENTS
	ChartsCore                  # for vtkAxis, vtkChart, vtkChartParallelCoordinates, used in FeatureScout, FuzzyFeatureTracking, GEMSE, FeatureAnalyzer
	CommonColor                 # for vtkNamedColors, vtkColorSeries, used in CompVis
	FiltersExtraction           # for vtkExtractGeometry, used in FIAKER - iASelectionInteractorStyle
	FiltersGeometry             # for vtkDataSetSurfaceFilter used in ExtractSurface - iAExtractSurfaceFilter
	FiltersHybrid               # for vtkDepthSortPolyData used in 4DCT, DreamCaster, FeatureScout, vtkPolyDataSilhouette used in FeatureScout
	FiltersModeling             # for vtkRotationalExtrusionFilter, vtkOutlineFilter
	FiltersStatistics           # for vtkPCAStatistics used in BoneThickness - iABoneThickness
	GUISupportQt                # for QVTKOpenGLNativeWidget
	ImagingHybrid               # for vtkSampleFunction used in FeatureScout - iABlobCluster
	InfovisLayout               # for vtkGraphLayoutStrategy used in CompVis
	#InteractionImage            # for vtkImageViewer2
	InteractionWidgets          # for vtkScalarBarWidget/Representation
	ImagingStatistics           # for vtkImageAccumulate
	IOGeometry                  # for vtkSTLReader/Writer
	IOMovie                     # for vtkGenericMovieWriter
	IOXML                       # for vtkXMLImageDataReader used in iAVTIFileIO
	RenderingAnnotation         # for vtkAnnotatedCubeActor, vtkCaptionActor, vtkScalarBarActor
	RenderingContextOpenGL2     # required, otherwise 3D renderer CRASHES somewhere with a nullptr access in vtkContextActor::GetDevice !!!
	RenderingImage              # for vtkImageResliceMapper used in Uncertainty - iAImageWidget
	RenderingVolumeOpenGL2      # for volume rendering
	RenderingQt                 # for vtkQImageToImageSource, also pulls in vtkGUISupportQt (for QVTKWidgetOpenGL)
	ViewsContext2D              # for vtkContextView, vtkContextInteractorStyle
	ViewsInfovis                # for vtkGraphItem
)
if ("${vtkRenderingOSPRay_LOADED}")
	add_compile_definitions(VTK_OSPRAY_AVAILABLE)
endif()

function (ExtractVersion filename identifier output_varname)
	file (STRINGS "${filename}" MYLINE REGEX "${identifier}")
	string(FIND "${MYLINE}" "=" MYLINE_EQUAL)
	string(LENGTH "${MYLINE}" MYLINE_LENGTH)
	math(EXPR MYVER_START "${MYLINE_EQUAL}+2")
	math(EXPR MYVER_LENGTH "${MYLINE_LENGTH}-${MYVER_START}-2")
	string(SUBSTRING "${MYLINE}" ${MYVER_START} ${MYVER_LENGTH} version_value)
	set(${output_varname} "${version_value}" PARENT_SCOPE)
endfunction()

if (RenderingOpenVR IN_LIST VTK_AVAILABLE_COMPONENTS)
	set(BUILD_INFO_VTK_OPENVR "on")
	list(APPEND VTK_COMPONENTS RenderingOpenVR)
	if (EXISTS "${OpenVR_INCLUDE_DIR}/openvr.h")
		# Parse OpenVR version number:
		ExtractVersion("${OpenVR_INCLUDE_DIR}/openvr.h" "k_nSteamVRVersionMajor" OpenVR_VERSION_MAJOR)
		ExtractVersion("${OpenVR_INCLUDE_DIR}/openvr.h" "k_nSteamVRVersionMinor" OpenVR_VERSION_MINOR)
		ExtractVersion("${OpenVR_INCLUDE_DIR}/openvr.h" "k_nSteamVRVersionBuild" OpenVR_VERSION_PATCH)
	endif()
	message(STATUS "    OpenVR: ${OpenVR_VERSION_MAJOR}.${OpenVR_VERSION_MINOR}.${OpenVR_VERSION_PATCH} in ${OpenVR_INCLUDE_DIR} (include dir)")
	string(REPLACE "/headers" "" OPENVR_PATH ${OpenVR_INCLUDE_DIR})
	if (WIN32)
		set(OPENVR_LIB_PATH "${OPENVR_PATH}/bin/win64")
	else()
		set(OPENVR_LIB_PATH "${OPENVR_PATH}/bin/linux64")
	endif()
	list(APPEND BUNDLE_DIRS "${OPENVR_LIB_PATH}")
else()
	set(BUILD_INFO_VTK_OPENVR "off")
	set(VTK_VR_OPTION_NAME "VTK_MODULE_ENABLE_VTK_RenderingOpenVR")
	message(STATUS "    OpenVR: NOT available! Enable ${VTK_VR_OPTION_NAME} in VTK to make it available.")
endif()
if (VTK_VERSION VERSION_GREATER_EQUAL "9.2.0")
	if (RenderingOpenXR IN_LIST VTK_AVAILABLE_COMPONENTS)
		set(BUILD_INFO_VTK_OPENXR "on")
		list(APPEND VTK_COMPONENTS RenderingOpenXR)
		find_package(OpenXR)    # basically only required for OpenXR_VERSION...
		message(STATUS "    OpenXR: ${OpenXR_VERSION_MAJOR}.${OpenXR_VERSION_MINOR}.${OpenXR_VERSION_PATCH} in ${OpenXR_INCLUDE_DIR} (include dir)")
		string(REPLACE "/include/" "" OPENXR_PATH ${OpenXR_INCLUDE_DIR})
		set(OPENXR_LIB_PATH "${OPENXR_PATH}/x64/bin")
		list(APPEND BUNDLE_DIRS "${OPENXR_LIB_PATH}")
	else()
		set(BUILD_INFO_VTK_OPENXR "off")
		set(VTK_VR_OPTION_NAME "VTK_MODULE_ENABLE_VTK_RenderingOpenXR")
		message(STATUS "    OpenXR: NOT available! Enable ${VTK_VR_OPTION_NAME} in VTK to make it available.")
	endif()
else()
	set(BUILD_INFO_VTK_OPENXR "N/A")
endif()
if ("theora" IN_LIST VTK_AVAILABLE_COMPONENTS)
	list(APPEND VTK_COMPONENTS theora)
endif()
if ("ogg" IN_LIST VTK_AVAILABLE_COMPONENTS)
	list(APPEND VTK_COMPONENTS ogg)
endif()
if ("IOOggTheora" IN_LIST VTK_AVAILABLE_COMPONENTS)
	list(APPEND VTK_COMPONENTS IOOggTheora)
endif()
find_package(VTK COMPONENTS ${VTK_COMPONENTS})
if (MSVC)
	set(VTK_LIB_DIR "${VTK_DIR}/bin/Release")
else()
	if (EXISTS "${VTK_DIR}/lib")
		set(VTK_LIB_DIR "${VTK_DIR}/lib")
	else()
		set(VTK_LIB_DIR "${VTK_DIR}/../../../lib")
	endif()
endif()
list(APPEND BUNDLE_DIRS "${VTK_LIB_DIR}")
option(VTK_USE_AVIWRITER "Enable usage of *.avi (an old Windows movie file format) writer. Note that enabling this might cause linker errors, since we cannot reliably determine whether VTK builds the required parts or not." OFF)
if ( vtkoggtheora_LOADED OR vtkogg_LOADED OR
     (VTK_ogg_FOUND EQUAL 1 AND VTK_theora_FOUND EQUAL 1 AND VTK_IOOggTheora_FOUND EQUAL 1) )
	message(STATUS "    Video: Ogg Theora Encoder available")
	set(VTK_VIDEO_SUPPORT "ogg")
else()
	message(WARNING "    Video: No encoder available! You will not be able to record videos.")
	set(VTK_VIDEO_SUPPORT "off")
endif()
set(BUILD_INFO_VTK_DETAILS "OpenVR: ${BUILD_INFO_VTK_OPENVR}, OpenXR: ${BUILD_INFO_VTK_OPENXR}, Video: ${VTK_VIDEO_SUPPORT}")
set(BUILD_INFO_VTK "VTK	${VTK_VERSION} (${BUILD_INFO_VTK_DETAILS})")
set(BUILD_INFO "${BUILD_INFO}    \"${BUILD_INFO_VTK}\\n\"\n")

# re-enable unmatched package name warning
unset(FPHSA_NAME_MISMATCHED)


# Qt (>= 6)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(QT_USE_QTXML TRUE)
if (QT_DIR AND NOT Qt6_DIR)
	set(Qt6_DIR "{QT_DIR}" CACHE PATH "" FORCE)
endif()
# not sure how the following works exactly currently - OpenGLWidgets is only available in Qt >= 6 I think...
#find_package(QT NAMES Qt6 COMPONENTS Widgets OpenGLWidgets REQUIRED)
# TO Do: Find way to automatically set QtxWidgets/Core/GuiTools for Qt >= 6
# Problem: Qt is discovered somehow above already, but inside vtk/itk discover code;
# maybe we could move this discovery earlier, but then we probably could not use
# the version used by vtk/itk...?
#if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
#	set(Qt${QT_VERSION_MAJOR}_DIR ${QT_DIR})
#	set(Qt${QT_VERSION_MAJOR}CoreTools_DIR ${QT_DIR}CoreTools CACHE PATH "" FORCE)
#	set(Qt${QT_VERSION_MAJOR}GuiTools_DIR ${QT_DIR}GuiTools CACHE PATH "" FORCE)
#	set(Qt${QT_VERSION_MAJOR}WidgetsTools_DIR ${QT_DIR}WidgetsTools CACHE PATH "" FORCE)
#endif
find_package(Qt6 COMPONENTS Concurrent Gui OpenGL Svg Widgets Xml REQUIRED)
message(STATUS "Qt: ${Qt6_VERSION} in ${Qt6_DIR}")
set(BUILD_INFO "${BUILD_INFO}    \"Qt	${Qt6_VERSION}\\n\"\n")
#if (Qt6_VERSION VERSION_LESS "6.0.0")
#	message(FATAL_ERROR "Your Qt version is too old. Please use Qt >= 6.0.0")
#endif()

string(REPLACE "/lib/cmake/Qt6" "" Qt_BASEDIR ${Qt6_DIR})
string(REPLACE "/cmake/Qt6" "" Qt_BASEDIR ${Qt_BASEDIR})	# on linux, lib is omitted if installed from package repos

if (WIN32)
	set(QT_LIB_DIR "${Qt_BASEDIR}/bin")
endif()
if (UNIX AND NOT APPLE AND NOT FLATPAK_BUILD)
	if (EXISTS "${Qt_BASEDIR}/lib")
		set(QT_LIB_DIR "${Qt_BASEDIR}/lib")
	else()
		set(QT_LIB_DIR "${Qt_BASEDIR}")
	endif()
endif()

# Qt Plugins
# find_package calls required due to https://bugreports.qt.io/browse/QTBUG-94066
# Install svg imageformats plugin:
if (FLATPAK_BUILD)
	# I guess plugins should all be available on Flatpak?
	#	install(FILES "$<TARGET_FILE:Qt5::QSvgPlugin>" DESTINATION bin/imageformats)
	#	install(FILES "$<TARGET_FILE:Qt5::QSvgIconPlugin>" DESTINATION bin/iconengines)
else()
	find_package(Qt6QSvgPlugin REQUIRED PATHS ${Qt6Gui_DIR})
	find_package(Qt6QSvgIconPlugin REQUIRED PATHS ${Qt6Gui_DIR})
	install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QSvgIconPlugin RUNTIME_DEPENDENCY_SET iADependencySet DESTINATION iconengines)
	install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QSvgPlugin RUNTIME_DEPENDENCY_SET iADependencySet DESTINATION imageformats)
endif()
# on windows, windows platform and style plugins are required:
if (WIN32)
#	find_package(Qt6QWindowsIntegrationPlugin REQUIRED PATHS ${Qt6Gui_DIR})
#	install(IMPORTED_RUNTIME_ARTIFACTS RUNTIME_DEPENDENCY_SET iADependencySet Qt6::QWindowsIntegrationPlugin DESTINATION platforms)
#	if (Qt6_VERSION VERSION_LESS "6.7")
#		find_package(Qt6QWindowsVistaStylePlugin REQUIRED PATHS ${Qt6Widgets_DIR})
#		install(IMPORTED_RUNTIME_ARTIFACTS RUNTIME_DEPENDENCY_SET iADependencySet Qt6::QWindowsVistaStylePlugin DESTINATION styles)
#	else()
#		find_package(Qt6QModernWindowsStylePlugin REQUIRED PATHS ${Qt6Widgets_DIR})
#		install(IMPORTED_RUNTIME_ARTIFACTS RUNTIME_DEPENDENCY_SET iADependencySet Qt6::QModernWindowsStylePlugin DESTINATION styles)
#	endif()
	find_package(Qt6QWindowsIntegrationPlugin REQUIRED PATHS ${Qt6Gui_DIR})
	install(FILES "$<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>" DESTINATION platforms)
	if (Qt6_VERSION VERSION_LESS "6.7")
		find_package(Qt6QWindowsVistaStylePlugin REQUIRED PATHS ${Qt6Widgets_DIR})
		install(FILES "$<TARGET_FILE:Qt6::QWindowsVistaStylePlugin>" DESTINATION styles)
	else()
		find_package(Qt6QModernWindowsStylePlugin REQUIRED PATHS ${Qt6Widgets_DIR})
		install(FILES "$<TARGET_FILE:Qt6::QModernWindowsStylePlugin>" DESTINATION styles)
	endif()
endif()
# on linux/unix, xcb platform plugin, and its plugins egl and glx are required:
if (UNIX AND NOT APPLE AND NOT FLATPAK_BUILD)
	find_package(Qt6QXcbIntegrationPlugin REQUIRED PATHS ${Qt6Gui_DIR})
	find_package(Qt6QXcbEglIntegrationPlugin REQUIRED PATHS ${Qt6Gui_DIR})
	find_package(Qt6QXcbGlxIntegrationPlugin REQUIRED PATHS ${Qt6Gui_DIR})
	install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbIntegrationPlugin RUNTIME_DEPENDENCY_SET iADependencySet DESTINATION platforms)
	install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbEglIntegrationPlugin RUNTIME_DEPENDENCY_SET iADependencySet DESTINATION xcbglintegrations)
	install(IMPORTED_RUNTIME_ARTIFACTS Qt6::QXcbGlxIntegrationPlugin RUNTIME_DEPENDENCY_SET iADependencySet DESTINATION xcbglintegrations)

	# install icu:
	# TODO: find out whether Qt was built with icu library dependencies
	# (typically only the case if webengine/webkit were included); but there
	# doesn't seem to be any CMake variable exposing this...
	#set(ICU_LIBS icudata icui18n icuuc)
	#foreach (ICU_LIB ${ICU_LIBS})
	#	set(ICU_LIB_LINK ${QT_LIB_DIR}/lib${ICU_LIB}.so)
	#	get_filename_component(ICU_SHAREDLIB "${ICU_LIB_LINK}" REALPATH)
	#	get_filename_component(ICU_SHAREDLIB_NAMEONLY "${ICU_SHAREDLIB}" NAME)
	#	string(LENGTH "${ICU_SHAREDLIB_NAMEONLY}" ICULIB_STRINGLEN)
	#	MATH(EXPR ICULIB_STRINGLEN "${ICULIB_STRINGLEN}-2")
	#	string(SUBSTRING "${ICU_SHAREDLIB_NAMEONLY}" 0 ${ICULIB_STRINGLEN} ICU_SHAREDLIB_DST)
	#	if (EXISTS "${ICU_SHAREDLIB}")
	#		install(FILES "${ICU_SHAREDLIB}" DESTINATION . RENAME "${ICU_SHAREDLIB_DST}")
	#	endif()
	#endforeach()
endif()
list(APPEND BUNDLE_DIRS "${QT_LIB_DIR}")


# Eigen
find_package(Eigen3)
if (EIGEN3_FOUND)
	message(STATUS "Eigen: ${EIGEN3_VERSION} in ${EIGEN3_INCLUDE_DIR}")
	set(BUILD_INFO "${BUILD_INFO}    \"Eigen	${EIGEN3_VERSION}\\n\"\n")
endif()


# HDF5
find_package(HDF5 NAMES hdf5 COMPONENTS C static NO_MODULE QUIET)
if (HDF5_FOUND)
	message(STATUS "HDF5: ${HDF5_VERSION} in ${HDF5_DIR}")
	set(BUILD_INFO "${BUILD_INFO}    \"HDF5	${HDF5_VERSION}\\n\"\n")
else()
	message(STATUS "HDF5: Not found - ${HDF5_NOT_FOUND_MESSAGE}")
endif()


# Astra Toolbox
find_package(AstraToolbox)
if (ASTRA_TOOLBOX_FOUND)
	message(STATUS "Astra Toolbox: ${ASTRA_VERSION} in ${ASTRA_TOOLBOX_DIR}")
	set(BUILD_INFO "${BUILD_INFO}    \"Astra	${ASTRA_VERSION}\\n\"\n")
	if (WIN32)
		set(ASTRA_LIB_DIR "${ASTRA_TOOLBOX_DIR}/bin/x64/Release_CUDA")
		if (NOT EXISTS "${ASTRA_LIB_DIR}/AstraCuda64.dll")
			get_filename_component(ASTRA_LIB_DIR "${ASTRA_TOOLBOX_LIBRARIES_RELEASE}" DIRECTORY)
		endif()
		if (NOT EXISTS "${ASTRA_LIB_DIR}/AstraCuda64.dll")
			message(WARNING "AstraCuda64.dll not found!")
		endif()
	elseif (UNIX AND NOT APPLE)
		get_filename_component(ASTRA_LIB_DIR "${ASTRA_TOOLBOX_LIBRARIES_RELEASE}" DIRECTORY)
	endif()
	list(APPEND BUNDLE_DIRS "${ASTRA_LIB_DIR}")
endif()


# OpenCL
find_package(OpenCLHeaders)
find_package(OpenCLHeadersCpp)
find_package(OpenCLICDLoader)
if (OpenCLICDLoader_FOUND)
	set(openiA_OPENCL_VERSION_OPTIONS "1.1.0" "1.2.0" "2.0.0" "2.1.0"  "2.2.0")
	list (FIND openiA_OPENCL_VERSION_OPTIONS "${openiA_OPENCL_VERSION}" opencl_version_index)
	if (${opencl_version_index} EQUAL -1)
		set(openiA_OPENCL_VERSION_DEFAULT "1.2.0")
		if (DEFINED openiA_OPENCL_VERSION)
			message(WARNING "Invalid openiA_OPENCL_VERSION, resetting to default ${openiA_OPENCL_VERSION_DEFAULT}!")
		endif()
		set(openiA_OPENCL_VERSION "${openiA_OPENCL_VERSION_DEFAULT}" CACHE STRING "The version of OpenCL to target (default: ${openiA_OPENCL_VERSION_DEFAULT})" FORCE)
		set_property(CACHE openiA_OPENCL_VERSION PROPERTY STRINGS ${openiA_OPENCL_VERSION_OPTIONS})
	endif()
	string(REPLACE "." "" CL_TARGET_OPENCL_VERSION "${openiA_OPENCL_VERSION}")
	add_library(OpenCLDefines INTERFACE)
	target_compile_definitions(OpenCLDefines INTERFACE
		__CL_ENABLE_EXCEPTIONS
		CL_HPP_ENABLE_EXCEPTIONS
		CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY
		CL_HPP_ENABLE_SIZE_T_COMPATIBILITY
		CL_HPP_TARGET_OPENCL_VERSION=${CL_TARGET_OPENCL_VERSION}
		CL_HPP_MINIMUM_OPENCL_VERSION=${CL_TARGET_OPENCL_VERSION}
		CL_TARGET_OPENCL_VERSION=${CL_TARGET_OPENCL_VERSION})
	if (openiA_OPENCL_VERSION MATCHES "^2")
		target_compile_definitions(OpenCLDefines INTERFACE CL_VERSION_2_0)
	else()
		target_compile_definitions(OpenCLDefines INTERFACE CL_USE_DEPRECATED_OPENCL_1_2_APIS)
	endif()
	target_link_libraries(OpenCLDefines INTERFACE OpenCL::Headers OpenCL::HeadersCpp OpenCL::OpenCL)
	message(STATUS "OpenCL: ${OpenCLICDLoader_DIR}")
	set(BUILD_INFO "${BUILD_INFO}    \"OpenCL targeted version	${openiA_OPENCL_VERSION}\\n\"\n")
	# new since CMake 3.21: installs library!
	install(IMPORTED_RUNTIME_ARTIFACTS OpenCL::OpenCL DESTINATION .)
	if (MSVC)
		string(REPLACE "share/cmake/OpenCLICDLoader" "bin" OpenCL_DLL_DIR "${OpenCLICDLoader_DIR}")
		#message(STATUS "OpenCL DLL dir: ${OpenCL_DLL_DIR}")
	endif()
endif()


# CUDA:
option(openiA_CUDA_ENABLED "Whether to enable search for CUDA toolkit. Default: enabled." ON)
if (openiA_CUDA_ENABLED)
	find_package(CUDAToolkit REQUIRED)
	if (CUDAToolkit_FOUND)
		message(STATUS "CUDA: ${CUDAToolkit_VERSION} in ${CUDAToolkit_TARGET_DIR}")
		set(BUILD_INFO "${BUILD_INFO}    \"CUDA	${CUDAToolkit_VERSION}\\n\"\n")
		#if (WIN32)
		# TODO: Check on Unix
		set(CUDA_LIB_DIR ${CUDAToolkit_BIN_DIR})
		#elseif (UNIX AND NOT APPLE)
		#	get_filename_component(CUDA_LIB_DIR "${CUDA_CUDART_LIBRARY}" DIRECTORY)
		#	get_filename_component(CUFFT_LIB_DIR "${CUDA_cufft_LIBRARY}" DIRECTORY)
		#	if (NOT "${CUDA_LIB_DIR}" STREQUAL "${CUFFT_LIB_DIR}")
		#		message(STATUS "CudaRT / CuFFT libs in different folders!")
		#		list(APPEND BUNDLE_DIRS "${CUFFT_LIB_DIR}")
		#	endif()
		#endif()
		list(APPEND BUNDLE_DIRS "${CUDA_LIB_DIR}")
	endif()
endif()

# OpenMP
include(${CMAKE_ROOT}/Modules/FindOpenMP.cmake)
# Above imports the target 'OpenMP::OpenMP_CXX'; only for CMake >= 3.9,
# but project-wide we require a higher version already anyway!


#-------------------------
# Compiler Flags
#-------------------------

set(openiA_AVX_SUPPORT_DISABLED "off")
set(openiA_AVX_SUPPORT_OPTIONS "${openiA_AVX_SUPPORT_DISABLED}" "AVX" "AVX2")
list (FIND openiA_AVX_SUPPORT_OPTIONS "${openiA_AVX_SUPPORT}" avx_support_index)
if (${avx_support_index} EQUAL -1)
	set(openiA_AVX_SUPPORT_DEFAULT ${openiA_AVX_SUPPORT_DISABLED})
	if (DEFINED openiA_AVX_SUPPORT)
		message(WARNING "Invalid openiA_AVX_SUPPORT, resetting to default ${openiA_AVX_SUPPORT_DEFAULT}!")
	endif()
	set(openiA_AVX_SUPPORT "${openiA_AVX_SUPPORT_DEFAULT}" CACHE STRING
		"AVX extensions to enable (default: ${openiA_AVX_SUPPORT_DEFAULT})." FORCE)
	set_property(CACHE openiA_AVX_SUPPORT PROPERTY STRINGS ${openiA_AVX_SUPPORT_OPTIONS})
endif()
set(BUILD_INFO "${BUILD_INFO}    \"Advanced Vector Extensions	${openiA_AVX_SUPPORT}\\n\"\n")

set(CMAKE_CXX_STANDARD 20)
# - C++20 can cause problems with ITK 5.0.1 (apparently in some experiments it wasn't yet fully C++20 compatible; though not sure on specifics)!
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES 0)  # otherwise we get errors such as /bin/sh: line 1: CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS-NOTFOUND: command not found
if (MSVC)
	# /MP                enable multi-processor compilation
	# /bigobj            increase the number of sections in .obj file (65,279 -> 2^32), exceeded by some compilations
	# /Zc:__cplusplus    set correct value in __cplusplus macro (https://learn.microsoft.com/en-us/cpp/build/reference/zc-cplusplus)
	# /Zc:inline         Remove unreferenced COMDAT (reduce .obj file size and improve linker speed, see https://learn.microsoft.com/en-us/cpp/build/reference/zc-inline-remove-unreferenced-comdat)
	# /utf-8             Enable using utf-8 as code page (https://learn.microsoft.com/en-us/cpp/build/reference/utf-8-set-source-and-executable-character-sets-to-utf-8?view=msvc-170)
	add_compile_options(/MP /bigobj /Zc:__cplusplus /Zc:inline /utf-8)
	if (MSVC_VERSION GREATER_EQUAL 1910)
		# specify standard conformance mode (https://docs.microsoft.com/en-us/cpp/build/reference/permissive-standards-conformance)
		add_compile_options(/permissive-)
	endif()

	########################################
	# More standard conformance...
	# Below version values deduced from https://cmake.org/cmake/help/latest/variable/MSVC_VERSION.html
	if (MSVC_VERSION GREATER_EQUAL 1925)  # Strict preprocessor available from VS 2019 16.5 - https://learn.microsoft.com/en-us/cpp/build/reference/zc-preprocessor
		add_compile_options(/Zc:preprocessor)
	endif()
	if (MSVC_VERSION GREATER_EQUAL 1929)  #  Address sanitizer available from VS 2019 16.9 - https://learn.microsoft.com/en-us/cpp/build/reference/fsanitize#
		option(openiA_ENABLE_ASAN  "Whether to enable the address sanitizer. Default: disabled." OFF)
		if (openiA_ENABLE_ASAN)
			set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /fsanitize=address")
			# vector and string are address-sanitizer aware also;
			# if not all statically linked libraries are compiled with address sanitizer, we get compilation errors:
			# LNK2038 mismatch detected for 'annotate_string' (/ 'annotate_vector'): value '0' doesn't mathc value'1'
			# 	https://learn.microsoft.com/en-us/answers/questions/864574/enabling-address-sanitizer-results-in-error-lnk203
			# 	https://learn.microsoft.com/en-us/cpp/sanitizers/error-container-overflow
			# Two options to address this problem:
			#      - enable address sanitizer for all statically linked libraries
			#      - disable annotations for string and vector by defining _DISABLE_VECTOR_ANNOTATION and _DISABLE_STRING_ANNOTATION
			add_compile_definitions(_DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
		endif()
	endif()
	if (MSVC_VERSION GREATER_EQUAL 1934)  # Enum type deduction available from VS 2022 17.4 - https://learn.microsoft.com/en-us/cpp/build/reference/zc-enumtypes
		add_compile_options(/Zc:enumTypes)
	endif()
	# ... and even more might be available, see see https://stackoverflow.com/questions/69575307 :
	# /volatile:iso /Zc:externConstexpr /Zc:throwingNew /Zc:templateScope
	########################################

	# Reduce size of .pdb files:
	option(openiA_COMPRESS_PDB "Whether to compress .pdb files to conserve disk space. Default: enabled." ON)
	if (openiA_COMPRESS_PDB)
		# significantly reduces size of .pdb files (89 -> 28 MB):
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /PDBCOMPRESS")
		# only slightly decrease build sizes (89 -> 80 MB), and disables incremental linking:
		#set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /OPT:REF /OPT:ICF")
	endif()

	if (NOT "${openiA_AVX_SUPPORT}" STREQUAL "${openiA_AVX_SUPPORT_DISABLED}")
		add_compile_options(/arch:${openiA_AVX_SUPPORT})
	endif()
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS)

	# enable all warnings, disable selected:
	add_compile_options(/W4 /wd4068 /wd4127 /wd4251 /wd4515)
	# disabled: C4068 - "unknown pragma - ignoring a pragma"
	#           C4127 - caused by QVector
	#           C4251 - "class requires dll interface"
	#           C4515 - "namespace uses itself" - caused by ITK/gdcm
else()
	# enable all warnings:
	add_compile_options(-Wall -Wextra) # with -Wpedantic, lots of warnings about extra ';' in VTK/ITK code...
endif()

if (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	add_compile_options(-pipe -fpermissive -fopenmp -march=core2 -O2 -msse4.2)
	add_link_options(-fopenmp)

	if (NOT "${openiA_AVX_SUPPORT}" STREQUAL "${openiA_AVX_SUPPORT_DISABLED}")
		string(TOLOWER "${openiA_AVX_SUPPORT}" openiA_AVX_SUPPORT_LOWER)
		add_compile_options(-m${openiA_AVX_SUPPORT_LOWER})
	endif()

	# we do need to set the RPATH to make lib load path recursive also be able to load dependent libraries from the rpath specified in the executables:
	# see https://stackoverflow.com/questions/58997230/cmake-project-fails-to-find-shared-library
	# strictly speaking, this is only needed for running the executables from the project folder
	# (as in an install, the RPATH of all installed executables and libraries is adapted anyway)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	add_compile_options(-Wcast-qual -Wfloat-zero-conversion -Wimplicit-fallthrough -Wmissing-variable-declarations -Wnewline-eof -Wshorten-64-to-32 -Wsuggest-override -Wzero-as-null-pointer-constant -Wcomma -Wunreachable-code-break -Wunreachable-code-return)
endif()

if (APPLE)
	# Mac OS X specific code
	message(WARNING "You are using MacOS - note that we do not regularly build on Mac OS, expect there to be some errors! Please report any issue you find at https://github.com/3dct/open_iA/issues!	")

	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa -framework OpenGL")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -framework Cocoa -framework OpenGL")
endif()


#-------------------------
# Visual Studio dll Paths
#-------------------------
if (MSVC)
	# Set up debugging/running environments	in Visual Studio to point to the correct dll files:
	cmake_path(NATIVE_PATH VTK_DIR VTK_WIN_DIR)
	cmake_path(NATIVE_PATH ITK_DIR ITK_WIN_DIR)
	cmake_path(NATIVE_PATH QT_LIB_DIR Qt_WIN_DIR)
	set(WinDLLPaths "${VTK_WIN_DIR}\\bin\\$(Configuration);${ITK_WIN_DIR}\\bin\\$(Configuration);${Qt_WIN_DIR}")

	if (OpenCLICDLoader_FOUND AND EXISTS "${OpenCL_DLL_DIR}")
		cmake_path(NATIVE_PATH OpenCL_DLL_DIR OpenCL_WIN_DIR)
		set(WinDLLPaths "${OpenCL_WIN_DIR};${WinDLLPaths}")
	endif()

	if (CUDAToolkit_FOUND)
		cmake_path(NATIVE_PATH CUDAToolkit_BIN_DIR CUDA_WIN_DIR)
		set(WinDLLPaths "${CUDA_WIN_DIR};${WinDLLPaths}")
	endif()

	if (ASTRA_TOOLBOX_FOUND)
		string(FIND ${ASTRA_TOOLBOX_LIBRARIES_RELEASE} "/" ASTRA_RELEASE_LIB_LASTSLASHPOS REVERSE)
		string(SUBSTRING ${ASTRA_TOOLBOX_LIBRARIES_RELEASE} 0 ${ASTRA_RELEASE_LIB_LASTSLASHPOS} ASTRA_LIBRARIES_RELEASE_PATH)
		cmake_path(NATIVE_PATH ASTRA_LIBRARIES_RELEASE_PATH ASTRA_LIBRARIES_RELEASE_WIN_PATH)
		set(WinDLLPaths "${ASTRA_LIBRARIES_RELEASE_WIN_PATH};${WinDLLPaths}")
	endif()

	if (NOT ("${ITKZLIB_LIBRARIES}" STREQUAL "itkzlib" OR "${ITKZLIB_LIBRARIES}" STREQUAL "zlib") )
		STRING (FIND "${ITKZLIB_LIBRARIES}" ";" SEMICOLONPOS)
		if (SEMICOLONPOS EQUAL -1)
			set(ZLIB_LIBRARY_RELEASE "${ITKZLIB_LIBRARIES}")
			set(ZLIB_LIBRARY_DEBUG "${ITKZLIB_LIBRARIES}")
		else()
			list(GET ITKZLIB_LIBRARIES 1 ZLIB_LIBRARY_RELEASE)
			list(GET ITKZLIB_LIBRARIES 3 ZLIB_LIBRARY_DEBUG)
		endif()
		string(FIND ${ZLIB_LIBRARY_RELEASE} "/" ZLIBRELLIB_LASTSLASHPOS REVERSE)
		string(SUBSTRING ${ZLIB_LIBRARY_RELEASE} 0 ${ZLIBRELLIB_LASTSLASHPOS} ZLIB_REL_LIB_DIR)
		string(FIND ${ZLIB_LIBRARY_DEBUG} "/" ZLIBDBGLIB_LASTSLASHPOS REVERSE)
		string(SUBSTRING ${ZLIB_LIBRARY_DEBUG} 0 ${ZLIBDBGLIB_LASTSLASHPOS} ZLIB_DBG_LIB_DIR)
		message(STATUS "ITK was built with system zlib, adding paths to dll. Release: ${ZLIB_REL_LIB_DIR}, Debug: ${ZLIB_DBG_LIB_DIR}")
		set(WinDLLPaths "${ZLIB_REL_LIB_DIR};${WinDLLPaths}")
	endif()

	if (RenderingOpenVR IN_LIST VTK_AVAILABLE_COMPONENTS)
		cmake_path(NATIVE_PATH OPENVR_LIB_PATH OPENVR_PATH_WIN)
		set(WinDLLPaths "${OPENVR_PATH_WIN};${WinDLLPaths}")
	endif()

	if (RenderingOpenXR IN_LIST VTK_AVAILABLE_COMPONENTS)
		cmake_path(NATIVE_PATH OPENXR_LIB_PATH OPENXR_PATH_WIN)
		set(WinDLLPaths "${OPENXR_PATH_WIN};${WinDLLPaths}")
	endif()

	if (openiA_ENABLE_ASAN)
		set(WinDLLPaths "$(VCToolsInstallDir)\\bin\\Hostx64\\x64;${WinDLLPaths}")
	endif()

	cmake_path(NATIVE_PATH CMAKE_BINARY_DIR CMAKE_BINARY_WIN_DIR)

	# Path to use in test environments (; has to be escaped)
	string(REPLACE ";" "\\;" TestDllTmp "${WinDLLPaths}")
	string(REPLACE "$(Configuration)" "Release" TestDllPaths "${TestDllTmp}")
	string(REPLACE ";" "\\;" EnvPathTmp "$ENV{PATH}")   # probably not strictly necessary?
	set(TestEnvPath "${TestDllPaths}\\;${EnvPathTmp}")
	# see tests/CMakeLists.txt or module/Segmentation/enabled.cmake for example usage
endif()


#-------------------------
# Common Settings
#-------------------------

# Options for default library symbol visibility - adds -fvisibility=hidden to libraries
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

option (openiA_USE_IDE_FOLDERS "Whether to group projects in subfolders in the IDE (mainly Visual Studio). Default: enabled." ON)
if (openiA_USE_IDE_FOLDERS)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CMake")
endif()

# open_iA Version number
include(GetGitRevisionDescription)
git_describe(openiA_VERSION openiA_HASH --tags)
if (FLATPAK_BUILD)
	# make sure version in appdata and in app match up (and in case git versioning doesn't work):
	execute_process(
		COMMAND "grep" "version=\"2" "at.zfp.openia.appdata.xml"
		COMMAND "cut" "-d" "\"" "-f2"
		COMMAND "head" "-n" "1"
		COMMAND "tr" "-d" "'\n'"
		OUTPUT_VARIABLE openiA_VERSION
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()
message(STATUS "Build version: ${openiA_VERSION}")
set(BUILD_INFO "${BUILD_INFO}    \"git revision	${openiA_HASH}\\n\"\n")

add_compile_definitions(UNICODE _UNICODE)    # Enable Unicode (probably not required anymore since Qt6 automatically defines these, see https://doc.qt.io/qt-6/cmake-qt5-and-qt6-compatibility.html#unicode-support-in-windows)

if (UNIX)
	set(CMAKE_INSTALL_RPATH "\$ORIGIN")      # Set RunPath in all created libraries / executables to $ORIGIN
	#    set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)
endif()

# Helper functions for adding libraries

# "old style" libraries (e.g. ITK or VTK < 9, with no imported targets)
# -> not working like this, since we have to use (I/V)TK_USE_FILE anyway for module autoinitialization
#function (ADD_LEGACY_LIBRARIES libname libprefix pubpriv liblist)
#	foreach (lib ${liblist})
#		set(fulllib "${libprefix}${lib}")
#		message(VERBOSE "    ${fulllib} - libs: ${${fulllib}_LIBRARIES}, include: ${${fulllib}_INCLUDE_DIRS}")
#		target_include_directories(${libname} ${pubpriv} ${${fulllib}_INCLUDE_DIRS})
#		target_link_libraries(${libname} ${pubpriv} ${${fulllib}_LIBRARIES})
#	endforeach()
#endfunction()

# "new style" libraries that bring in all dependencies automatically, and that only need to be linked to
function (ADD_IMPORTEDTARGET_LIBRARIES libname libprefix pubpriv liblist)
	foreach (lib ${liblist})
		set(fulllib "${libprefix}${lib}")
		message(VERBOSE "    ${fulllib}")
		target_link_libraries(${libname} ${pubpriv} ${fulllib})
	endforeach()
endfunction()

function (ADD_VTK_LIBRARIES libname pubpriv liblist)
	ADD_IMPORTEDTARGET_LIBRARIES(${libname} ${VTK_LIB_PREFIX} ${pubpriv} "${liblist}")
endfunction()
