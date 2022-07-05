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
set(BUILD_INFO "\"CMake: ${CMAKE_VERSION} (Generator: ${CMAKE_GENERATOR})\\n\"\n")
if (MSVC)
	message(STATUS "Compiler: Visual C++ (MSVC_VERSION ${MSVC_VERSION} / ${CMAKE_CXX_COMPILER_VERSION})")
	set(BUILD_INFO "${BUILD_INFO}    \"Compiler: Visual C++ (MSVC_VERSION ${MSVC_VERSION} / ${CMAKE_CXX_COMPILER_VERSION})\\n\"\n")
	set(BUILD_INFO "${BUILD_INFO}    \"Windows SDK: ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}\\n\"\n")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	message(STATUS "Compiler: Clang (${CMAKE_CXX_COMPILER_VERSION})")
	set(BUILD_INFO "${BUILD_INFO}    \"Compiler: Clang (Version ${CMAKE_CXX_COMPILER_VERSION})\\n\"\n")
elseif (CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "Compiler: G++ (${CMAKE_CXX_COMPILER_VERSION})")
	set(BUILD_INFO "${BUILD_INFO}    \"Compiler: G++ (Version ${CMAKE_CXX_COMPILER_VERSION})\\n\"\n")
else()
	message(WARNING "Unknown compiler! Please report any CMake or compilation errors on https://github.com/3dct/open_iA!")
	set(BUILD_INFO "${BUILD_INFO}    \"Compiler: Unknown\\n\"\n")
endif()
set(BUILD_INFO "${BUILD_INFO}    \"Targetting ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}\\n\"\n")
if (FLATPAK_BUILD)
	set(BUILD_INFO "${BUILD_INFO}    \"Flatpak Build\\n\"\n")
endif()

#-------------------------
# Output Directories
#-------------------------
if (CMAKE_CONFIGURATION_TYPES)
	message(STATUS "Multi-configuration generator")
	# On windows, both executable and dll's are considered RUNTIME, as well as executables on Mac OS...
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/x64/Debug")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/x64/Release")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/x64/RelWithDebInfo")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/x64/MinSizeRel")
	if (APPLE)	# but .dylib's on Mac OS are considered LIBRARY (XCode generator - multi-config)
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/x64/Debug")
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/x64/Release")
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/x64/RelWithDebInfo")
		set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/x64/MinSizeRel")
	endif()
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

# Prepare empty BUNDLE vars:
set(BUNDLE_DIRS "")
set(BUNDLE_LIBS "")

# Suppress CMake warnings
#    - triggered for OpenMP package
#    - searched for within ITK and VTK
#    - see https://cmake.org/cmake/help/v3.17/module/FindPackageHandleStandardArgs.html
set(FPHSA_NAME_MISMATCHED 1)

# ITK
set(SAVED_CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}")
find_package(ITK REQUIRED)
message(STATUS "ITK: ${ITK_VERSION} in ${ITK_DIR}")
if (ITK_VERSION VERSION_LESS "4.10.0")
	message(FATAL_ERROR "Your ITK version is too old. Please use ITK >= 4.10")
endif()
set(ITK_COMPONENTS
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
if (ITK_VERSION VERSION_GREATER "4.12.99") # libraries split up in ITK 4.13:
	list(APPEND ITK_COMPONENTS ITKImageIO)
	list(APPEND ITK_COMPONENTS ITKIORAW)  # apparently not included in ITKImageIO
else()
	foreach (mod IN LISTS ITK_MODULES_ENABLED)
		if (${mod} MATCHES "IO")
			list(APPEND ITK_COMPONENTS ${mod})
		endif()
	endforeach()
endif()
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
include(${ITK_USE_FILE}) # <- maybe avoid by using include/link commands on targets instead? -> ITK build system would have to be updated for that first, 5.x does not support it!
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
message(STATUS "    ITK_LIB_DIR: ${ITK_LIB_DIR}")
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
if ("${ITKGPUCommon_LIBRARY_DIRS}" STREQUAL "")
	set(ITK_GPU_INFO "off")
	message(WARNING "ITK is built without GPU support (flag ITK_USE_GPU disabled). Some GPU-optimized functionality might not be available!")
endif()
set(BUILD_INFO "${BUILD_INFO}    \"ITK: ${ITK_VERSION} (GPU: ${ITK_GPU_INFO}, SCIFIO: ${ITK_SCIFIO_INFO}, RTK: ${ITK_RTK_INFO}, HOAG: ${ITK_HGrad_INFO})\\n\"\n")

# VTK
find_package(VTK REQUIRED)
message(STATUS "VTK: ${VTK_VERSION} in ${VTK_DIR}")
if (VTK_VERSION VERSION_LESS "8.0.0")
	message(FATAL_ERROR "Your VTK version is too old. Please use VTK >= 8.0")
endif()
if (VTK_VERSION VERSION_LESS "8.2.0" AND "${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL")
	message(FATAL_ERROR "VTK was built with the old OpenGL backend; please switch to VTK_RENDERING_BACKEND=OpenGL2, or use a newer VTK version (VTK >= 8.2.0)!")
endif()
set(VTK_LIB_PREFIX "VTK::")
if (VTK_VERSION VERSION_LESS "9.0.0")
	set(VTK_COMP_PREFIX "vtk")
	set(VTK_BASE_LIB_LIST kwiml)
	set(VTK_LIB_PREFIX "vtk")
else()
	set(VTK_COMP_PREFIX "")
endif()
set(VTK_COMPONENTS
	${VTK_COMP_PREFIX}FiltersModeling         # for vtkRotationalExtrusionFilter, vtkOutlineFilter
	${VTK_COMP_PREFIX}InteractionImage        # for vtkImageViewer2
	${VTK_COMP_PREFIX}InteractionWidgets      # for vtkScalarBarWidget/Representation
	${VTK_COMP_PREFIX}ImagingStatistics       # for vtkImageAccumulate
	${VTK_COMP_PREFIX}IOGeometry              # for vtkSTLReader/Writer
	${VTK_COMP_PREFIX}IOMovie                 # for vtkGenericMovieWriter
	${VTK_COMP_PREFIX}RenderingAnnotation     # for vtkAnnotatedCubeActor, vtkCaptionActor, vtkScalarBarActor
	${VTK_COMP_PREFIX}RenderingContextOpenGL2 # required, otherwise 3D renderer CRASHES somewhere with a nullptr access in vtkContextActor::GetDevice !!!
	${VTK_COMP_PREFIX}RenderingImage          # for vtkImageResliceMapper
	${VTK_COMP_PREFIX}RenderingVolumeOpenGL2  # for volume rendering
	${VTK_COMP_PREFIX}RenderingQt             # for vtkQImageToImageSource, also pulls in vtkGUISupportQt (for QVTKWidgetOpenGL)
	${VTK_COMP_PREFIX}ViewsContext2D          # for vtkContextView, vtkContextInteractorStyle
	${VTK_COMP_PREFIX}ViewsInfovis)           # for vtkGraphItem
if (VTK_MAJOR_VERSION GREATER_EQUAL 9)
	list(APPEND VTK_COMPONENTS          # components not pulled in automatically anymore in VTK >= 9:
		ChartsCore                  # for vtkAxis, vtkChart, vtkChartParallelCoordinates, used in FeatureScout, FuzzyFeatureTracking, GEMSE, PorosityAnalyzer
		CommonColor                 # for vtkNamedColors, vtkColorSeries, used in CompVis
		CommonComputationalGeometry # for vtkParametricSpline, used in core - iASpline/iAParametricSpline
		FiltersExtraction           # for vtkExtractGeometry, used in FIAKER - iASelectionInteractorStyle
		FiltersGeometry             # for vtkImageDataGeometryFilter used in iALabel3D and vtkDataSetSurfaceFilter used in ExtractSurface - iAExtractSurfaceFilter
		FiltersHybrid               # for vtkDepthSortPolyData used in 4DCT, DreamCaster, FeatureScout, vtkPolyDataSilhouette used in FeatureScout
		FiltersStatistics           # for vtkDataSetSurfaceFilter used in BoneThickness - iABoneThickness
		GUISupportQt                # for QVTKOpenGLNativeWidget
		ImagingHybrid               # for vtkSampleFunction.h used in FeatureScout - iABlobCluster
		InfovisLayout               # for vtkGraphLayoutStrategy used in CompVis
		IOXML                       # for vtkXMLImageDataReader used in iAIO
	)
else()
	list(APPEND VTK_COMPONENTS
		vtkFiltersPoints            # for vtkExtractSurface in FiAKEr
		vtkFiltersProgrammable)     # for vtkProgrammableGlyphFilter in CompVis
endif()
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

if (vtkRenderingOpenVR_LOADED OR TARGET VTK::RenderingOpenVR)
	set(BUILD_INFO_VTK_VR_SUPPORT "on")
	list(APPEND VTK_COMPONENTS ${VTK_COMP_PREFIX}RenderingOpenVR)
	if (VTK_MAJOR_VERSION LESS 9)
		string(FIND "${vtkRenderingOpenVR_INCLUDE_DIRS}" ";" semicolonpos REVERSE)
		math(EXPR aftersemicolon "${semicolonpos}+1")
		string(SUBSTRING "${vtkRenderingOpenVR_INCLUDE_DIRS}" ${aftersemicolon} -1 OpenVR_INCLUDE_DIR)
	# no else required as VTK >= 9 requires OpenVR_INCLUDE_DIR to be set anyway
	endif()
	if (EXISTS "${OpenVR_INCLUDE_DIR}/openvr.h")
		# Parse OpenVR version number:
		ExtractVersion("${OpenVR_INCLUDE_DIR}/openvr.h" "k_nSteamVRVersionMajor" OPENVR_VERSION_MAJOR)
		ExtractVersion("${OpenVR_INCLUDE_DIR}/openvr.h" "k_nSteamVRVersionMinor" OPENVR_VERSION_MINOR)
		ExtractVersion("${OpenVR_INCLUDE_DIR}/openvr.h" "k_nSteamVRVersionBuild" OPENVR_VERSION_BUILD)
	endif()
	message(STATUS "    OpenVR: ${OPENVR_VERSION_MAJOR}.${OPENVR_VERSION_MINOR}.${OPENVR_VERSION_BUILD} in ${OpenVR_INCLUDE_DIR} (include dir)")
	string(REGEX REPLACE "/headers" "" OPENVR_PATH ${OpenVR_INCLUDE_DIR})
	if (WIN32)
		set(OPENVR_LIB_PATH "${OPENVR_PATH}/bin/win64")
	else()
		set(OPENVR_LIB_PATH "${OPENVR_PATH}/bin/linux64")
	endif()
	list(APPEND BUNDLE_DIRS "${OPENVR_LIB_PATH}")
else()
	set(BUILD_INFO_VTK_VR_SUPPORT "off")
	if (VTK_MAJOR_VERSION LESS 8)
		set(VTK_VR_OPTION_NAME "Module_vtkRenderingOpenVR")
	else()
		set(VTK_VR_OPTION_NAME "VTK_MODULE_ENABLE_VTK_RenderingOpenVR")
	endif()
	message(STATUS "    RenderingOpenVR: NOT available! Enable ${VTK_VR_OPTION_NAME} in VTK to make it available.")
endif()
if (VTK_MAJOR_VERSION GREATER 8)
	if ("theora" IN_LIST VTK_AVAILABLE_COMPONENTS)
		list(APPEND VTK_COMPONENTS theora)
	endif()
	if ("ogg" IN_LIST VTK_AVAILABLE_COMPONENTS)
		list(APPEND VTK_COMPONENTS ogg)
	endif()
	if ("IOOggTheora" IN_LIST VTK_AVAILABLE_COMPONENTS)
		list(APPEND VTK_COMPONENTS IOOggTheora)
	endif()
endif()
find_package(VTK COMPONENTS ${VTK_COMPONENTS})
if (VTK_MAJOR_VERSION LESS 9)		# VTK >= 9.0 uses imported targets -> include directories are set by target_link_libraries(... VTK_LIBRARIES) call!
	INCLUDE(${VTK_USE_FILE})
endif()
if (MSVC)
	set(VTK_LIB_DIR "${VTK_DIR}/bin/Release")
else()
	if (EXISTS "${VTK_DIR}/lib")
		set(VTK_LIB_DIR "${VTK_DIR}/lib")
	else()
		set(VTK_LIB_DIR "${VTK_DIR}/../../../lib")
	endif()
endif()
message(STATUS "    VTK_LIB_DIR: ${VTK_LIB_DIR}")
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
set(BUILD_INFO_VTK_DETAILS "${BUILD_INFO_VTK_DETAILS}OpenVR: ${BUILD_INFO_VTK_VR_SUPPORT}, Video: ${VTK_VIDEO_SUPPORT}")
set(BUILD_INFO_VTK "VTK: ${VTK_VERSION} (${BUILD_INFO_VTK_DETAILS})")
set(BUILD_INFO "${BUILD_INFO}    \"${BUILD_INFO_VTK}\\n\"\n")

# re-enable unmatched package name warning
unset(FPHSA_NAME_MISMATCHED)


# Qt (>= 5)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(QT_USE_QTXML TRUE)
if (Qt5_DIR AND NOT QT_DIR)
	set(QT_DIR "${Qt5_DIR}" CACHE PATH "" FORCE)
endif()
if (Qt6_DIR AND NOT QT_DIR)
	set(QT_DIR "{Qt6_DIR}" CACHE PATH "" FORCE)
endif()
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets OpenGLWidgets REQUIRED)
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
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Concurrent Gui OpenGL Svg Widgets Xml REQUIRED)
message(STATUS "Qt: ${QT_VERSION} in ${Qt${QT_VERSION_MAJOR}_DIR}")
set(BUILD_INFO "${BUILD_INFO}    \"Qt: ${QT_VERSION}\\n\"\n")
if (QT_VERSION VERSION_LESS "5.9.0")
	message(FATAL_ERROR "Your Qt version is too old. Please use Qt >= 5.9")
endif()

string(REGEX REPLACE "/lib/cmake/Qt${QT_VERSION_MAJOR}" "" Qt_BASEDIR ${Qt${QT_VERSION_MAJOR}_DIR})
string(REGEX REPLACE "/cmake/Qt${QT_VERSION_MAJOR}" "" Qt_BASEDIR ${Qt_BASEDIR})	# on linux, lib is omitted if installed from package repos

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

# Install svg imageformats plugin:
if (FLATPAK_BUILD)
	# I guess plugins should all be available on Flatpak?
	#	INSTALL (FILES "$<TARGET_FILE:Qt5::QSvgPlugin>" DESTINATION bin/imageformats)
	#	INSTALL (FILES "$<TARGET_FILE:Qt5::QSvgIconPlugin>" DESTINATION bin/iconengines)
else()
	if (${QT_VERSION_MAJOR} GREATER_EQUAL 6) # Qt6 does not expose plugins? at least not the same as in Qt 5
		set(LIB_SvgIconPlugin "${Qt_BASEDIR}/plugins/iconengines/${CMAKE_SHARED_LIBRARY_PREFIX}qsvgicon${CMAKE_SHARED_LIBRARY_SUFFIX}")
		set(LIB_SvgPlugin "${Qt_BASEDIR}/plugins/imageformats/${CMAKE_SHARED_LIBRARY_PREFIX}qsvg${CMAKE_SHARED_LIBRARY_SUFFIX}")
		INSTALL (FILES "${LIB_SvgIconPlugin}" DESTINATION iconengines)
		list(APPEND BUNDLE_LIBS "${LIB_SvgIconPlugin}")
		INSTALL (FILES "${LIB_SvgPlugin}" DESTINATION imageformats)
		list(APPEND BUNDLE_LIBS "${LIB_SvgPlugin}")
	else() # use imported targets & generator expressions:
		INSTALL (FILES "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QSvgIconPlugin>" DESTINATION iconengines)
		list(APPEND BUNDLE_LIBS "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QSvgIconPlugin>")
		INSTALL (FILES "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QSvgPlugin>" DESTINATION imageformats)
		list(APPEND BUNDLE_LIBS "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QSvgPlugin>")
	endif()
endif()
# on windows, windows platform and vista style plugins are required:
if (WIN32)
	if (${QT_VERSION_MAJOR} GREATER_EQUAL 6) # Qt6 does not expose plugins? at least not the same as in Qt 5
		set(LIB_WindowsPlatform "${Qt_BASEDIR}/plugins/platforms/qwindows.dll")
		set(LIB_WindowsVistaStyle "${Qt_BASEDIR}/plugins/styles/qwindowsvistastyle.dll")
		INSTALL (FILES "${LIB_WindowsPlatform}" DESTINATION platforms)
		INSTALL (FILES "${LIB_WindowsVistaStyle}" DESTINATION styles)
	else() # use imported targets & generator expressions:
		INSTALL (FILES "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QWindowsIntegrationPlugin>" DESTINATION platforms)
		INSTALL (FILES "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QWindowsVistaStylePlugin>" DESTINATION styles)
	endif()
endif()
# on linux/unix, xcb platform plugin, and its plugins egl and glx are required:
if (UNIX AND NOT APPLE AND NOT FLATPAK_BUILD)
	if (${QT_VERSION_MAJOR} GREATER_EQUAL 6) # Qt6 does not expose plugins? at least not the same as in Qt 5
		set(LIB_XcbPlatform "${Qt_BASEDIR}/plugins/platforms/libqxcb.so")
		set(LIB_XcbEglIntegration "${Qt_BASEDIR}/plugins/xcbglintegrations/libqxcb-egl-integration.so")
		set(LIB_XcbGlxIntegration "${Qt_BASEDIR}/plugins/xcbglintegrations/libqxcb-glx-integration.so")
		INSTALL (FILES "${LIB_XcbPlatform}" DESTINATION platforms)
		INSTALL (FILES "${LIB_XcbEglIntegration}" DESTINATION xcbglintegrations)
		INSTALL (FILES "${LIB_XcbGlxIntegration}" DESTINATION xcbglintegrations)
	else()
		INSTALL (FILES "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QXcbIntegrationPlugin>" DESTINATION platforms)
		INSTALL (FILES "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QXcbEglIntegrationPlugin>" DESTINATION xcbglintegrations)
		INSTALL (FILES "$<TARGET_FILE:Qt${QT_VERSION_MAJOR}::QXcbGlxIntegrationPlugin>" DESTINATION xcbglintegrations)
	endif()

	# install icu:
	# TODO: find out whether Qt was built with icu library dependencies
	# (typically only the case if webengine/webkit were included); but there
	# doesn't seem to be any CMake variable exposing this...
	set(ICU_LIBS icudata icui18n icuuc)
	foreach (ICU_LIB ${ICU_LIBS})
		set(ICU_LIB_LINK ${QT_LIB_DIR}/lib${ICU_LIB}.so)
		get_filename_component(ICU_SHAREDLIB "${ICU_LIB_LINK}" REALPATH)
		get_filename_component(ICU_SHAREDLIB_NAMEONLY "${ICU_SHAREDLIB}" NAME)
		string(LENGTH "${ICU_SHAREDLIB_NAMEONLY}" ICULIB_STRINGLEN)
		MATH(EXPR ICULIB_STRINGLEN "${ICULIB_STRINGLEN}-2")
		string(SUBSTRING "${ICU_SHAREDLIB_NAMEONLY}" 0 ${ICULIB_STRINGLEN} ICU_SHAREDLIB_DST)
		if (EXISTS "${ICU_SHAREDLIB}")
			INSTALL (FILES "${ICU_SHAREDLIB}" DESTINATION . RENAME "${ICU_SHAREDLIB_DST}")
		endif()
	endforeach()
endif()
list(APPEND BUNDLE_DIRS "${QT_LIB_DIR}")


# Eigen
find_package(Eigen3)
if (EIGEN3_FOUND)
	message(STATUS "Eigen: ${EIGEN3_VERSION} in ${EIGEN3_INCLUDE_DIR}")
	set(BUILD_INFO "${BUILD_INFO}    \"Eigen: ${EIGEN3_VERSION}\\n\"\n")
endif()


# HDF5
# ToDo: Check for whether hdf5 is build as shared or static library,
# prefer static but also enable utilization of shared?
find_package(HDF5 NAMES hdf5 COMPONENTS C NO_MODULE QUIET)

if (HDF5_FOUND)
	set(HDF5_CORE_LIB_NAME libhdf5${CMAKE_STATIC_LIBRARY_SUFFIX})
	set(HDF5_SZIP_LIB_NAME libszip${CMAKE_STATIC_LIBRARY_SUFFIX})
	set(HDF5_SZIP_LIB_ALT_NAME libsz${CMAKE_STATIC_LIBRARY_SUFFIX})
	set(HDF5_Z_LIB_NAME libzlib${CMAKE_STATIC_LIBRARY_SUFFIX})
	set(HDF5_Z_LIB_ALT_NAME libz${CMAKE_STATIC_LIBRARY_SUFFIX})
	find_path(HDF5_INCLUDE_OVERWRITE_DIR hdf5.h PATHS "${HDF5_DIR}/../../include" "${HDF5_DIR}/../../../include")
	set(HDF5_INCLUDE_DIR "${HDF5_INCLUDE_OVERWRITE_DIR}" CACHE PATH "" FORCE)
	unset(HDF5_INCLUDE_OVERWRITE_DIR CACHE)
	find_library(HDF5_CORE_LIB ${HDF5_CORE_LIB_NAME} PATHS ${HDF5_DIR}/../../lib ${HDF5_DIR}/../../../lib)
	find_library(HDF5_Z_LIB ${HDF5_Z_LIB_NAME} NAMES ${HDF5_Z_LIB_ALT_NAME} PATHS ${HDF5_DIR}/../../lib ${HDF5_DIR}/../../../lib NO_DEFAULT_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_PATH NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_FIND_ROOT_PATH)
	find_library(HDF5_SZIP_LIB ${HDF5_SZIP_LIB_NAME} NAMES ${HDF5_SZIP_LIB_ALT_NAME} PATHS ${HDF5_DIR}/../../lib ${HDF5_DIR}/../../../lib)
	set(HDF5_LIB_LIST ${HDF5_CORE_LIB} ${HDF5_CORE_HL_LIB} ${HDF5_TOOL_LIB} ${HDF5_SZIP_LIB} ${HDF5_Z_LIB})
	# HDF5 version parts seem to be broken (e.g. for 1.12.1, I get MAJOR=1.12, MINOR=1, PATCH=1), so we have to split full version string ourselves:
	string(REPLACE "." ";" HDF5_VERSION_LIST ${HDF5_VERSION})
	list(GET HDF5_VERSION_LIST 0 HDF5_VERSION_MAJOR)
	list(GET HDF5_VERSION_LIST 1 HDF5_VERSION_MINOR)
	list(GET HDF5_VERSION_LIST 2 HDF5_VERSION_PATCH)
	# HDF5 >= 1.10.7 / 1.12.1 split off a part of the sz library into a separate "aec" library...
	if ( (${HDF5_VERSION_MINOR} EQUAL 10 AND "${HDF5_VERSION}" VERSION_GREATER_EQUAL "1.10.7") OR
         (${HDF5_VERSION_MINOR} EQUAL 12 AND "${HDF5_VERSION}" VERSION_GREATER_EQUAL "1.12.1") )
		set(HDF5_AEC_LIB_NAME libaec${CMAKE_STATIC_LIBRARY_SUFFIX})
		find_library(HDF5_AEC_LIB ${HDF5_AEC_LIB_NAME} PATHS ${HDF5_DIR}/../../lib ${HDF5_DIR}/../../../lib)
		list(APPEND HDF5_LIB_LIST ${HDF5_AEC_LIB})
		unset(HDF5_AEC_LIB CACHE)
	endif()
	set(HDF5_LIBRARY ${HDF5_LIB_LIST} CACHE STRING "" FORCE)
	unset(HDF5_Z_LIB CACHE)
	unset(HDF5_SZIP_LIB CACHE)
	unset(HDF5_CORE_LIB CACHE)
	message(STATUS "HDF5: ${HDF5_VERSION} in ${HDF5_DIR}")
	set(BUILD_INFO "${BUILD_INFO}    \"HDF5: ${HDF5_VERSION}\\n\"\n")
else()
	message(STATUS "HDF5: Not found")
endif()


# Astra Toolbox
find_package(AstraToolbox)
if (ASTRA_TOOLBOX_FOUND)
	string(FIND "${ASTRA_TOOLBOX_DIR}" "-" ASTRA_DASH_POS REVERSE)
	set(ASTRA_VERSION "unknown version")
	if (${ASTRA_DASH_POS} GREATER -1)
		MATH(EXPR ASTRA_DASH_POS "${ASTRA_DASH_POS} + 1")
		string(SUBSTRING "${ASTRA_TOOLBOX_DIR}" ${ASTRA_DASH_POS} -1 ASTRA_VERSION)
	endif()
	message(STATUS "Astra Toolbox: ${ASTRA_VERSION} in ${ASTRA_TOOLBOX_DIR}")
	set(BUILD_INFO "${BUILD_INFO}    \"Astra: ${ASTRA_VERSION}\\n\"\n")
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
find_package(OpenCL)
if (OPENCL_FOUND)
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
	add_library(OpenCL INTERFACE)
	target_compile_definitions(OpenCL INTERFACE __CL_ENABLE_EXCEPTIONS
		CL_HPP_TARGET_OPENCL_VERSION=${CL_TARGET_OPENCL_VERSION}
		CL_TARGET_OPENCL_VERSION=${CL_TARGET_OPENCL_VERSION})
	if (openiA_OPENCL_VERSION MATCHES "^2")
		target_compile_definitions(OpenCL INTERFACE CL_VERSION_2_0)
	else()
		target_compile_definitions(OpenCL INTERFACE CL_USE_DEPRECATED_OPENCL_1_2_APIS)
	endif()
	target_link_libraries(OpenCL INTERFACE ${OPENCL_LIBRARIES})
	target_include_directories(OpenCL INTERFACE ${OPENCL_INCLUDE_DIRS} ${Toolkit_DIR}/OpenCL)
	message(STATUS "OpenCL: include=${OPENCL_INCLUDE_DIRS}, libraries=${OPENCL_LIBRARIES}")
	set(BUILD_INFO "${BUILD_INFO}    \"OpenCL targeted version: ${openiA_OPENCL_VERSION}\\n\"\n")
	if (WIN32)
		# Find path of OpenCL.dll to include in release:
		get_filename_component(OPENCL_LIB_DIR "${OPENCL_LIBRARIES}" DIRECTORY)
		get_filename_component(OPENCL_LIB_BASENAME "${OPENCL_LIBRARIES}" NAME_WE)
		
		if (EXISTS "${OPENCL_LIB_DIR}/${OPENCL_LIB_BASENAME}.dll")
			list(APPEND BUNDLE_DIRS "${OPENCL_LIB_DIR}")
		else()
			string(REGEX REPLACE "lib" "bin" OPENCL_BIN_DIR "${OPENCL_LIB_DIR}")
			if (EXISTS "${OPENCL_BIN_DIR}/${OPENCL_LIB_BASENAME}.dll")
				list(APPEND BUNDLE_DIRS "${OPENCL_BIN_DIR}")
			else()
				message(STATUS "Directory containing ${OPENCL_LIB_BASENAME}.dll was not found. You can continue building, but the program might not run (or it might fail to run when installed/cpacked).")
			endif()
		endif()
	elseif (UNIX AND NOT APPLE)
		# typically OPENCL_LIBRARIES will only contain the one libOpenCL.so anyway, FOREACH just to make sure
		foreach (OPENCL_LIB ${OPENCL_LIBRARIES})
			get_filename_component(OPENCL_LIB_DIR "${OPENCL_LIB}" DIRECTORY)
			list(APPEND BUNDLE_DIRS "${OPENCL_LIB_DIR}")
		endforeach()
	endif()
endif()


# CUDA:
option(openiA_CUDA_ENABLED "Whether to enable search for CUDA toolkit. Default: enabled." ON)
if (openiA_CUDA_ENABLED)
	find_package(CUDA)
	if (CUDA_FOUND)
		message(STATUS "CUDA: ${CUDA_VERSION} in ${CUDA_TOOLKIT_ROOT_DIR}")
		set(BUILD_INFO "${BUILD_INFO}    \"CUDA: ${CUDA_VERSION}\\n\"\n")
		if (WIN32)
			set(CUDA_LIB_DIR ${CUDA_TOOLKIT_ROOT_DIR}/bin)
		elseif (UNIX AND NOT APPLE)
			get_filename_component(CUDA_LIB_DIR "${CUDA_CUDART_LIBRARY}" DIRECTORY)
			get_filename_component(CUFFT_LIB_DIR "${CUDA_cufft_LIBRARY}" DIRECTORY)
			if (NOT "${CUDA_LIB_DIR}" STREQUAL "${CUFFT_LIB_DIR}")
				message(STATUS "CudaRT / CuFFT libs in different folders!")
				list(APPEND BUNDLE_DIRS "${CUFFT_LIB_DIR}")
			endif()
		endif()
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
set(BUILD_INFO "${BUILD_INFO}    \"Advanced Vector Extensions support: ${openiA_AVX_SUPPORT}\\n\"\n")

if (${QT_VERSION_MAJOR} GREATER_EQUAL 6)
	# Qt 6 requires C++ 17, but causes problems with ITK 4.12.2 (throw clauses -> "ISO c++1z does not allow dynamic exception specifications")
	if (ITK_VERSION VERSION_LESS "5.0.0")
		MESSAGE(SEND_ERROR "You have chosen Qt >= 6.0 in combination with ITK <= 5.0. "
			"Qt >= 6 requires to use the C++17 standard, but ITK <= 5 does not work with C++17. "
			"Please either choose a Qt version < 6.0 or an ITK version >= 5.0!")
	endif()
	set(CMAKE_CXX_STANDARD 17)
else()
	set(CMAKE_CXX_STANDARD 14)
endif()
# - C++20 can cause problems with ITK 5.0.1 (apparently in some experiments it wasn't yet fully C++20 compatible; though not sure on specifics)!
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
	# /bigobj            increase the number of sections in .obj file (65,279 -> 2^32), exceeded by some compilations
	# /Zc:__cplusplus    set correct value in __cplusplus macro (https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus)
	# /MP                enable multi-processor compilation
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /bigobj /Zc:__cplusplus")
	if (MSVC_VERSION GREATER_EQUAL 1910)
		# specify standard conformance mode (https://docs.microsoft.com/en-us/cpp/build/reference/permissive-standards-conformance)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /permissive-")
	endif()

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
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS
		_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING	# silence warnings when compiling VTK (<= 9.0.1) with C++17
	)
	
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

# check: are CMAKE_C_FLAGS really required or are CMAKE_CXX_FLAGS alone enough?
if (CMAKE_COMPILER_IS_GNUCXX)
	if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb3")
	endif()
endif()

if (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fpermissive -fopenmp -march=core2 -O2 -msse4.2")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -fopenmp -march=core2 -O2 -msse4.2")

	if (NOT "${openiA_AVX_SUPPORT}" STREQUAL "${openiA_AVX_SUPPORT_DISABLED}")
		string(TOLOWER "${openiA_AVX_SUPPORT}" openiA_AVX_SUPPORT_LOWER)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m${openiA_AVX_SUPPORT_LOWER}")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m${openiA_AVX_SUPPORT_LOWER}")
	endif()

	# we do need to set the RPATH to make lib load path recursive also be able to load dependent libraries from the rpath specified in the executables:
	# see https://stackoverflow.com/questions/58997230/cmake-project-fails-to-find-shared-library
	# strictly speaking, this is only needed for running the executables from the project folder
	# (as in an install, the RPATH of all installed executables and libraries is adapted anyway)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")
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
	string(REGEX REPLACE "/" "\\\\" VTK_WIN_DIR ${VTK_DIR})
	string(REGEX REPLACE "/" "\\\\" ITK_WIN_DIR ${ITK_DIR})
	string(REGEX REPLACE "/" "\\\\" Qt_WIN_DIR ${QT_LIB_DIR})
	set(WinDLLPaths "${VTK_WIN_DIR}\\bin\\$(Configuration);${ITK_WIN_DIR}\\bin\\$(Configuration);${Qt_WIN_DIR}")
	
	if (OPENCL_FOUND AND EXISTS "${OPENCL_DLL}")
		string(REGEX REPLACE "/OpenCL.dll" "" OPENCL_WIN_DIR ${OPENCL_DLL})
		string(REGEX REPLACE "/" "\\\\" OPENCL_WIN_DIR ${OPENCL_WIN_DIR})
		set(WinDLLPaths "${OPENCL_WIN_DIR};${WinDLLPaths}")
	endif()

	if (CUDA_FOUND)
		string(REGEX REPLACE "/" "\\\\" CUDA_WIN_DIR ${CUDA_TOOLKIT_ROOT_DIR})
		set(WinDLLPaths "${CUDA_WIN_DIR}\\bin;${WinDLLPaths}")
	endif()

	if (ITK_USE_SYSTEM_FFTW)
		set(WinDLLPaths "${ITK_FFTW_LIBDIR};${WinDLLPaths}")
	endif()

	if (ASTRA_TOOLBOX_FOUND)
		string(FIND ${ASTRA_TOOLBOX_LIBRARIES_RELEASE} "/" ASTRA_RELEASE_LIB_LASTSLASHPOS REVERSE)
		string(SUBSTRING ${ASTRA_TOOLBOX_LIBRARIES_RELEASE} 0 ${ASTRA_RELEASE_LIB_LASTSLASHPOS} ASTRA_LIBRARIES_RELEASE_PATH)
		string(REGEX REPLACE "/" "\\\\" ASTRA_LIBRARIES_RELEASE_WIN_PATH ${ASTRA_LIBRARIES_RELEASE_PATH})
		set(WinDLLPaths "${ASTRA_LIBRARIES_RELEASE_WIN_PATH};${WinDLLPaths}")
	endif()

	if (NOT "${ITKZLIB_LIBRARIES}" STREQUAL "itkzlib")
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

	if (HDF5_FOUND)
		string(REGEX REPLACE "/cmake/hdf5" "" HDF5_BASE_DIR ${HDF5_DIR})
		string(REGEX REPLACE "/" "\\\\" HDF5_BASE_DIR ${HDF5_BASE_DIR})
		if (EXISTS "${HDF5_BASE_DIR}\\bin\\Release")
			set(WinDLLPaths "${HDF5_BASE_DIR}\\bin\\$(Configuration);${WinDLLPaths}")
		else()
			set(WinDLLPaths "${HDF5_BASE_DIR}\\bin;${WinDLLPaths}")
		endif()
	endif()

	if (vtkRenderingOpenVR_LOADED OR TARGET VTK::RenderingOpenVR)
		string(REGEX REPLACE "/" "\\\\" OPENVR_PATH_WIN ${OPENVR_LIB_PATH})
		set(WinDLLPaths "${OPENVR_PATH_WIN};${WinDLLPaths}")
	endif()

	if (ONNX_RUNTIME_LIBRARIES)
		get_filename_component(ONNX_LIB_DIR ${ONNX_RUNTIME_LIBRARIES} DIRECTORY)
		string(REGEX REPLACE "/" "\\\\" ONNX_LIB_WIN_DIR ${ONNX_LIB_DIR})
		set(WinDLLPaths "${ONNX_LIB_WIN_DIR};${WinDLLPaths}")
	endif()

	string(REGEX REPLACE "/" "\\\\" CMAKE_BINARY_WIN_DIR ${CMAKE_BINARY_DIR})
endif()

if (ONNX_RUNTIME_LIBRARIES)
	get_filename_component(ONNX_LIB_DIR ${ONNX_RUNTIME_LIBRARIES} DIRECTORY)
	list(APPEND BUNDLE_DIRS "${ONNX_LIB_DIR}")
endif()


#-------------------------
# Common Settings
#-------------------------

option (openiA_USE_IDE_FOLDERS "Whether to group projects in subfolders in the IDE (mainly Visual Studio). Default: enabled." ON)
if (openiA_USE_IDE_FOLDERS)
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
	set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "_CMake")
endif()

# open_iA Version number
include(GetGitRevisionDescription)
git_describe(openiA_VERSION openiA_HASH --tags)
message(STATUS "Build version: ${openiA_VERSION}")
set(BUILD_INFO "${BUILD_INFO}    \"git revision: ${openiA_HASH}\\n\"\n")

add_compile_definitions(UNICODE _UNICODE)    # Enable Unicode

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
#		if (openiA_DEPENDENCY_INFO)
#			message(STATUS "    ${fulllib} - libs: ${${fulllib}_LIBRARIES}, include: ${${fulllib}_INCLUDE_DIRS}")
#		endif()
#		target_include_directories(${libname} ${pubpriv} ${${fulllib}_INCLUDE_DIRS})
#		target_link_libraries(${libname} ${pubpriv} ${${fulllib}_LIBRARIES})
#	endforeach()
#endfunction()

# "new style" libraries that bring in all dependencies automatically, and that only need to be linked to
function (ADD_IMPORTEDTARGET_LIBRARIES libname libprefix pubpriv liblist)
	foreach (lib ${liblist})
		set(fulllib "${libprefix}${lib}")
		if (openiA_DEPENDENCY_INFO)
			message(STATUS "    ${fulllib}")
		endif()
		target_link_libraries(${libname} ${pubpriv} ${fulllib})
	endforeach()
endfunction()

function (ADD_VTK_LIBRARIES libname pubpriv liblist)
	if (VTK_VERSION VERSION_LESS "9.0.0")
		#list(APPEND liblist ${VTK_BASE_LIB_LIST})
		#ADD_LEGACY_LIBRARIES(${libname} ${VTK_LIB_PREFIX} ${pubpriv} "${liblist}")
	else()
		ADD_IMPORTEDTARGET_LIBRARIES(${libname} ${VTK_LIB_PREFIX} ${pubpriv} "${liblist}")
	endif()
endfunction()
