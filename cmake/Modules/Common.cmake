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
INCLUDE (CTest)
enable_testing()

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

# ITK (>= 4)
FIND_PACKAGE(ITK)
IF(ITK_FOUND)
	INCLUDE(${ITK_USE_FILE})
#	INCLUDE_DIRECTORIES($ENV{ITK_APPS_SRC_PATH}/QtITK )
ELSE(ITK_FOUND)
	MESSAGE(FATAL_ERROR "Cannot build without ITK.  Please set ITK_DIR.")
ENDIF(ITK_FOUND)
IF(ITK_VERSION_MAJOR LESS 4)
	MESSAGE(FATAL_ERROR "Your ITK version is too old. Please use ITK >= 4.x")
ENDIF (ITK_VERSION_MAJOR LESS 4)
SET( ITK_LIBRARIES
	ITKBiasCorrection		ITKBioCell				ITKCommon			ITKIOImageBase
	ITKFEM					ITKIOBioRad				ITKIOBMP			ITKIOGDCM			ITKIOGE
	ITKIOGIPL				ITKIOHDF5				ITKIOIPL			ITKIOJPEG			ITKIOLSM
	ITKIOMeta				ITKIONIFTI				ITKIONRRD			ITKIOPNG			ITKIOSiemens
	ITKIOSpatialObjects		ITKIOStimulate			ITKIOTIFF			ITKIOVTK			ITKIOXML
	ITKVtkGlue				ITKKLMRegionGrowing		ITKMesh				ITKOptimizers		ITKPath
	ITKVNLInstantiation		ITKVTK					ITKWatersheds		ITKDICOMParser		ITKEXPAT
	ITKLabelMap				itkjpeg					ITKMetaIO			itkNetlibSlatec
	ITKniftiio				ITKNrrdIO				itkpng				itksys
	itktiff					itkv3p_netlib			itkvcl				itkvnl				itkvnl_algo
	ITKznz
)
IF ("${ITKZLIB_LIBRARIES}" STREQUAL "itkzlib")
	SET (ITK_LIBRARIES ${ITK_LIBRARIES} itkzlib)
ENDIF()
IF (NOT ${ITKGPUCommon_LIBRARY_DIRS} STREQUAL "")
	# cannot use ITKGPUCommon_LOADED - it is always defined - bug?
	SET( ITK_LIBRARIES  ${ITK_LIBRARIES}
		ITKGPUAnisotropicSmoothing
		ITKGPUCommon
		ITKGPUFiniteDifference
		ITKGPUImageFilterBase
		ITKGPUSmoothing
		ITKGPUThresholding)
ENDIF()
IF (ITK_USE_SYSTEM_FFTW)
	SET(ITK_LIBRARIES  ${ITK_LIBRARIES} ITKFFT)
	IF (MSVC)
		IF (NOT ITK_USE_FFTWF)
			MESSAGE(SEND_ERROR "Required flag ITK_USE_FFTWF not enabled in ITK CMake configuration; please rebuild ITK with this flag enabled!")
		ENDIF()
		SET(FFTW_DLL ${ITK_FFTW_LIBDIR}/libfftw3f-3.dll)
		MESSAGE(STATUS "ITK_USE_SYSTEM_FFTW is enabled, thus installing dll file: ${FFTW_DLL}")
		INSTALL (FILES ${FFTW_DLL} DESTINATION .)
	ELSE(MSVC)
		MESSAGE(WARNING "ITK_USE_SYSTEM_FFTW is enabled, but the installation of the appropriate shared lib wasn't implemented yet!")
		#INSTALL (FILES ${ITK_FFTW_LIBDIR}/libfftw3f-3.so DESTINATION .)
	ENDIF (MSVC)
ENDIF (ITK_USE_SYSTEM_FFTW)
IF (ITK_VERSION_MAJOR LESS 5 AND ITK_VERSION_MINOR LESS 12)
	# apparently, in 4.12 the itkopenjpeg.lib isn't built anymore by default
	SET (ITK_LIBRARIES ${ITK_LIBRARIES} itkopenjpeg)
ENDIF()
IF(ITK_VERSION_MAJOR GREATER 4 OR ITK_VERSION_MINOR GREATER 4)
		# starting with ITK 4.5, there is an implicit dependency on ITKIOMRC:
	SET(ITK_LIBRARIES ${ITK_LIBRARIES} ITKIOMRC)
	IF (SCIFIO_LOADED)
		ADD_DEFINITIONS( -DUSE_SCIFIO )
		MESSAGE(STATUS "ITK has SCIFIO support enabled. Notice that in order to run a build with this library on another machine than the one you built it, the environment variable SCIFIO_PATH has to be set to the path containing the SCIFIO jar files! Otherwise loading images will fail!")
		SET (SCIFIO_PATH "${ITK_DIR}/lib/jars")
		IF (MSVC)
			# variable will be set to the debugging environment instead of copying (see gui/CMakeLists.txt)
		ELSE(MSVC)
			MESSAGE(STATUS "Copying SCIFIO jars from ${SCIFIO_PATH} to ${DESTDIR}")
			SET (DESTDIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scifio_jars")
			configure_file("${SCIFIO_PATH}/bioformats_package.jar" "${DESTDIR}/bioformats_package.jar" COPYONLY)
			configure_file("${SCIFIO_PATH}/scifio-itk-bridge.jar" "${DESTDIR}/scifio-itk-bridge.jar" COPYONLY)
		ENDIF(MSVC)
		INSTALL(FILES "${SCIFIO_PATH}/bioformats_package.jar" DESTINATION scifio_jars)
		INSTALL(FILES "${SCIFIO_PATH}/scifio-itk-bridge.jar" DESTINATION scifio_jars)
		SET(ITK_LIBRARIES ${ITK_LIBRARIES} SCIFIO)
	ENDIF(SCIFIO_LOADED)
ELSE ()
	SET(ITK_LIBRARIES ${ITK_LIBRARIES} ITKReview)
ENDIF(ITK_VERSION_MAJOR GREATER 4 OR ITK_VERSION_MINOR GREATER 4)


# VTK (>= 6)
FIND_PACKAGE(VTK)
IF(VTK_FOUND)
	INCLUDE(${VTK_USE_FILE})
	include_directories(${VTK_INCLUDE_DIRS})
ELSE(VTK_FOUND)
	MESSAGE(FATAL_ERROR "Cannot build without VTK.  Please set VTK_DIR.")
ENDIF(VTK_FOUND)
IF(VTK_VERSION_MAJOR LESS 6)
	MESSAGE(FATAL_ERROR "Your VTK version is too old. Please use VTK >= 6.0")
ENDIF(VTK_VERSION_MAJOR LESS 6)
SET (VTK_LIBRARIES
	vtkCommonCore
	vtkChartsCore
	vtkDICOMParser
	vtkFiltersCore
	vtkGUISupportQt
	vtkGUISupportQtOpenGL
	vtkImagingCore
	vtkImagingStatistics
	vtkInfovisCore
	vtkIOCore
	vtkIOMovie
	vtkIOGeometry
	vtkIOXML
	vtkRenderingContext2D
	vtkRenderingImage
	vtkViewsCore
	vtkViewsInfovis
	vtksys)
IF ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
	ADD_DEFINITIONS(-DVTK_OPENGL2_BACKEND)
ENDIF("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
SET (VTK_LIBRARIES ${VTK_LIBRARIES}	vtkRendering${VTK_RENDERING_BACKEND})
IF (VTK_VERSION_MAJOR GREATER 6 OR VTK_VERSION_MINOR GREATER 0)
	SET (VTK_LIBRARIES ${VTK_LIBRARIES}
		vtkRenderingCore
		vtkRenderingFreeType
		vtkRenderingQt
		vtkViewsContext2D)
	SET (VTK_LIBRARIES ${VTK_LIBRARIES}	vtkRenderingVolume${VTK_RENDERING_BACKEND})
ENDIF (VTK_VERSION_MAJOR GREATER 6 OR  VTK_VERSION_MINOR GREATER 0)
IF (VTK_VERSION_MAJOR GREATER 6 OR VTK_VERSION_MINOR GREATER 1)
	SET (VTK_LIBRARIES ${VTK_LIBRARIES}	vtkRenderingContext${VTK_RENDERING_BACKEND})
ENDIF (VTK_VERSION_MAJOR GREATER 6 OR VTK_VERSION_MINOR GREATER 1)
IF (VTK_VERSION_MAJOR GREATER 7 OR (VTK_VERSION_MAJOR EQUAL 7 AND VTK_VERSION_MINOR GREATER 0))
	SET(VTK_LIBRARIES ${VTK_LIBRARIES} vtkFiltersHybrid vtkFiltersModeling vtkImagingHybrid vtkRenderingAnnotation)
	IF ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
		SET (VTK_LIBRARIES ${VTK_LIBRARIES}	vtkRenderingGL2PSOpenGL2 vtkgl2ps)
	ENDIF ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
ENDIF (VTK_VERSION_MAJOR GREATER 7 OR (VTK_VERSION_MAJOR EQUAL 7 AND VTK_VERSION_MINOR GREATER 0))
IF (vtkoggtheora_LOADED)
	MESSAGE(STATUS "Video: Ogg Theora Encoder available")
	ADD_DEFINITIONS(-DVTK_USE_OGGTHEORA_ENCODER)
ENDIF(vtkoggtheora_LOADED)


# Qt (>= 5)
SET(CMAKE_AUTOMOC ON)
SET(QT_USE_QTXML TRUE)
IF (WIN32)
	SET( CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "C:/Program Files (x86)/Windows Kits/8.1/Lib/winv6.3/um/x64" )
ENDIF (WIN32)
FIND_PACKAGE(Qt5 COMPONENTS Widgets Xml Network Test OpenGL PrintSupport REQUIRED)
# Qt5OpenGL_INCLUDE_DIRS seems to be required on linux only, but doesn't hurt on Windows:
INCLUDE_DIRECTORIES(${Qt5Widgets_INCLUDE_DIRS} ${Qt5OpenGL_INCLUDE_DIRS} )
SET(QT_LIBRARIES ${Qt5Core_LIBRARIES} ${Qt5Xml_LIBRARIES} ${Qt5OpenGL_LIBRARIES} ${Qt5Network_LIBRARIES} ${Qt5PrintSupport_LIBRARIES})


# Eigen
FIND_PACKAGE(Eigen3)
IF(EIGEN3_FOUND)
	ADD_DEFINITIONS( -DUSE_EIGEN )
	INCLUDE_DIRECTORIES( ${EIGEN3_INCLUDE_DIR} )
ENDIF(EIGEN3_FOUND)


# Astra Toolbox
FIND_PACKAGE(AstraToolbox)


# CUDA:
FIND_PACKAGE(CUDA)


# OpenCL
FIND_PACKAGE(OpenCL)


#OpenMP
INCLUDE(${CMAKE_ROOT}/Modules/FindOpenMP.cmake)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")


#-------------------------
# Library Installation
#-------------------------

# ITK
SET (ITK_VER "${ITK_VERSION_MAJOR}.${ITK_VERSION_MINOR}")
IF (WIN32)
	SET (ITK_LIB_DIR "${ITK_DIR}/bin/Release")
	# strangely, under Windows, ITK seems to build a completely different set of shared libraries
	# than under Linux. Those listed below are required by our binary. Even more strangely, this
	# list is different from the list of libraries required for linking)
	SET (WIN_ITK_LIBS ${WIN_ITK_LIBS}
		ITKCommon	ITKIOBioRad	ITKIOBMP	ITKIOGIPL	ITKIOImageBase	ITKIOPNG	ITKIOStimulate
		ITKIOVTK	ITKIOGDCM	ITKIOGE	ITKIOIPL	ITKIOHDF5	ITKIOJPEG	ITKIOLSM	ITKIOMRC	ITKIOTIFF
		ITKIOMeta	ITKIONIFTI	ITKIONRRD	ITKLabelMap	ITKOptimizers	ITKStatistics	ITKTransform
		ITKVTK	ITKWatersheds
	)
	IF (SCIFIO_LOADED)
		SET (WIN_ITK_LIBS ${WIN_ITK_LIBS} itkSCIFIO)
	ENDIF()
	SET (ITK_LIB_DIR "${ITK_DIR}/bin/Release")
	FOREACH(ITK_LIB ${WIN_ITK_LIBS})
		INSTALL (FILES ${ITK_LIB_DIR}/${ITK_LIB}-${ITK_VER}.dll DESTINATION .)
	ENDFOREACH(ITK_LIB)
ENDIF(WIN32)
IF (UNIX)
	SET (EXTRA_ITK_LIBS	itkdouble-conversion
		itkgdcmcharls	itkgdcmCommon	itkgdcmDICT	itkgdcmDSED	itkgdcmIOD	itkgdcmjpeg12
		itkgdcmjpeg16	itkgdcmjpeg8	itkgdcmMSFF	itkgdcmopenjpeg itkgdcmuuid
		itknetlib	ITKSpatialObjects
		ITKStatistics	ITKTransform)
	# starting with ITK 4.11, itkhdf5* libraries must not be referenced anymore, before they are required:
	IF(ITK_VERSION_MAJOR LESS 5 AND ITK_VERSION_MINOR LESS 11)
		SET(EXTRA_ITK_LIBS ${EXTRA_ITK_LIBS} itkhdf5_cpp itkhdf5)
	ENDIF()
	SET (ALL_ITK_LIBS ${ITK_LIBRARIES} ${EXTRA_ITK_LIBS})
	SET (ITK_LIB_DIR "${ITK_DIR}/lib")
	FOREACH(ITK_LIB ${ALL_ITK_LIBS})
	# hack: SCIFIO apparently needs to be linked as "SCIFIO" but the lib is called "itkSCFICIO"...
		STRING(REPLACE "SCIFIO" "itkSCIFIO" ITK_LIBF "${ITK_LIB}")
		INSTALL (FILES ${ITK_LIB_DIR}/lib${ITK_LIBF}-${ITK_VER}.so.1 DESTINATION .)
	ENDFOREACH(ITK_LIB)
ENDIF(UNIX)

# VTK
SET (VTK_VER "${VTK_VERSION_MAJOR}.${VTK_VERSION_MINOR}")
SET (VTK_EXTRA_LIBS
	vtkalglib	vtkCommonColor	vtkCommonComputationalGeometry	vtkCommonDataModel
	vtkCommonExecutionModel	vtkCommonMath	vtkCommonMisc	vtkCommonSystem
	vtkCommonTransforms	vtkexoIIc	vtkexpat	vtkFiltersExtraction
	vtkFiltersGeneral	vtkFiltersGeometry	vtkFiltersImaging	vtkFiltersSources
	vtkFiltersStatistics	vtkFiltersTexture	vtkfreetype	vtkhdf5
	vtkImagingColor	vtkImagingFourier	vtkImagingGeneral	vtkImagingHybrid
	vtkImagingSources	vtkInfovisLayout	vtkInteractionStyle	vtkInteractionWidgets
	vtkIOImage	vtkIOLegacy	vtkIOXMLParser	vtkjpeg	vtklibxml2
	vtkmetaio	vtkoggtheora	vtkpng	vtkRenderingLabel
	vtkRenderingVolume	vtktiff	vtkverdict	vtkViewsInfovis
	vtkzlib)
IF (${VTK_MAJOR_VERSION} LESS 7 AND ${VTK_MINOR_VERSION} LESS 3)
	SET (VTK_EXTRA_LIBS ${VTK_EXTRA_LIBS}	vtkRenderingFreeTypeOpenGL)
ENDIF()
SET (VTK_ALL_LIBS ${VTK_LIBRARIES} ${VTK_EXTRA_LIBS})
IF (WIN32)
	SET (VTK_LIB_DIR "${VTK_DIR}/bin/Release")
	SET (VTK_WIN_EXTRA_LIBS vtkImagingMath)
	IF ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
		SET (VTK_WIN_EXTRA_LIBS ${VTK_WIN_EXTRA_LIBS} vtkglew)
	ENDIF ()
	SET (VTK_ALL_WIN_LIBS ${VTK_ALL_LIBS} ${VTK_WIN_EXTRA_LIBS})
	FOREACH(VTK_LIB ${VTK_ALL_WIN_LIBS})
		INSTALL (FILES ${VTK_LIB_DIR}/${VTK_LIB}-${VTK_VER}.dll DESTINATION .)
	ENDFOREACH(VTK_LIB)
	INSTALL(FILES ${VTK_LIB_DIR}/QVTKWidgetPlugin.dll DESTINATION .)
ENDIF(WIN32)
IF (UNIX)
	SET (VTK_LIB_DIR "${VTK_DIR}/lib")
	FOREACH(VTK_LIB ${VTK_ALL_LIBS})
		INSTALL (FILES ${VTK_LIB_DIR}/lib${VTK_LIB}-${VTK_VER}.so.1 DESTINATION .)
	ENDFOREACH(VTK_LIB)
	INSTALL(FILES ${VTK_LIB_DIR}/libQVTKWidgetPlugin.so DESTINATION .)
ENDIF(UNIX)


# Qt
STRING(REGEX REPLACE "/lib/cmake/Qt5" "" Qt5_BASEDIR ${Qt5_DIR})
SET (QT_COMMON_EXTRA_LIBS Qt5Gui Qt5Widgets)
SET (QT_ALL_LIBS ${QT_LIBRARIES} ${QT_COMMON_EXTRA_LIBS})
IF(WIN32)
	SET (QT_LIB_DIR "${Qt5_BASEDIR}/bin")
	FOREACH(QT_LIBCOLON ${QT_ALL_LIBS})
		STRING (REPLACE "::" "" QT_LIB "${QT_LIBCOLON}")
		INSTALL (FILES ${QT_LIB_DIR}/${QT_LIB}.dll DESTINATION .)
	ENDFOREACH(QT_LIBCOLON)
	INSTALL (FILES ${Qt5_BASEDIR}/plugins/platforms/qwindows.dll DESTINATION platforms)
ENDIF(WIN32)
IF(UNIX)
	SET (QT_LIB_DIR "${Qt5_BASEDIR}/lib")
	SET (QT_VER "${Qt5Core_VERSION}")
	SET (QT_SHORTVER "${Qt5Core_VERSION_MAJOR}")
	SET (QT_LINUX_EXTRA_LIBS Qt5DBus Qt5XcbQpa)
	SET (QT_LINUX_ALL_LIBS ${QT_ALL_LIBS} ${QT_LINUX_EXTRA_LIBS})
	FOREACH(QT_LIBCOLON ${QT_LINUX_ALL_LIBS})
		STRING (REPLACE "::" "" QT_LIB "${QT_LIBCOLON}")
		INSTALL (FILES ${QT_LIB_DIR}/lib${QT_LIB}.so.${QT_VER} DESTINATION . RENAME lib${QT_LIB}.so.${QT_SHORTVER})
	ENDFOREACH(QT_LIBCOLON)

	IF (EXISTS "${QT_LIB_DIR}/libQt5X11Extras.so.${QT_VER}" )
		INSTALL (FILES ${QT_LIB_DIR}/${QT_LIB_DIR}/libQt5X11Extras.so.${QT_VER} DESTINATION . RENAME libQt5X11Extras.so.${QT_SHORTVER})
	ELSE ()
		STRING(REGEX REPLACE "/qtbase" "" Qt5_BINDIR ${Qt5_BASEDIR})
		INSTALL (FILES ${Qt5_BINDIR}/qtx11extras/lib/libQt5X11Extras.so.${QT_VER} DESTINATION . RENAME libQt5X11Extras.so.${QT_SHORTVER})
	ENDIF ()

	# xcb platform plugin:
	INSTALL (FILES ${Qt5_BASEDIR}/plugins/platforms/libqxcb.so DESTINATION platforms)
	# egl and glx plugins to the xcb platform plugin:
	INSTALL (DIRECTORY ${Qt5_BASEDIR}/plugins/xcbglintegrations DESTINATION .)

	# install icu:
	# TODO: find out whether Qt was built with icu library dependencies
	# (typically only the case if webengine/webkit were included); but there
	# doesn't seem to be any CMake variable exposing this...
	# also, no version information on ICU is available, therefore the hard-coded 56
	SET(ICU_LIBS icudata icui18n icuuc)
	SET(ICU_VER "56")
	FOREACH(ICU_LIB ${ICU_LIBS})
		SET (ICU_LIB_FILE ${QT_LIB_DIR}/lib${ICU_LIB}.so.${ICU_VER}.1)
		IF (EXISTS "${ICU_LIB_FILE}")
			INSTALL (FILES ${ICU_LIB_FILE} DESTINATION . RENAME lib${ICU_LIB}.so.${ICU_VER})
		ENDIF()
	ENDFOREACH()
ENDIF(UNIX)

# OpenCL:
IF (OPENCL_FOUND)
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

IF (ASTRA_TOOLBOX_FOUND)
	MESSAGE(STATUS "ASTRA Toolbox installation still to do...")
ENDIF()

IF (CUDA_FOUND)
	MESSAGE(STATUS "CUDA runtime installation still to do...")
ENDIF()

#-------------------------
# Compiler Flags
#-------------------------
IF (WIN32)
	ADD_DEFINITIONS(-DCL_COMP) # TODO: check if that really is used for something!
ENDIF (WIN32)
IF (MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /EHsc")  # multi-processor compilation and common exception handling strategy
	ADD_DEFINITIONS(/D _CRT_SECURE_NO_WARNINGS)
	ADD_DEFINITIONS(/D _SCL_SECURE_NO_WARNINGS)
ENDIF (MSVC)
IF (CMAKE_COMPILER_IS_GNUCXX)

	# sets the c++14 standard (or the next lower one which is available on the compiler)
	SET(CMAKE_CXX_STANDARD 14)
	SET(CMAKE_CXX_EXTENSIONS OFF)
	# Make sure at least C++ 0x is supported:
	INCLUDE (CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
	IF (NOT COMPILER_SUPPORTS_CXX0X)
		MESSAGE(WARN "The used compiler ${CMAKE_CXX_COMPILER} has no C++0x/11 support. Please use a newer C++ compiler.")
	ENDIF()

	execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
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
		#set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
		#MESSAGE (STATUS "${CMAKE_C_FLAGS}")
	ENDIF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

ENDIF (CMAKE_COMPILER_IS_GNUCXX)


#-------------------------
# Common Settings
#-------------------------

SET (CORE_LIBRARY_NAME open_iA_Core)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)    # Enable Unicode

SET(CMAKE_INSTALL_RPATH "\$ORIGIN")      # Set RunPath in all created libraries / executables to $ORIGIN
