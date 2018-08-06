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

# ITK (>= 4)
FIND_PACKAGE(ITK)
IF(ITK_FOUND)
	INCLUDE(${ITK_USE_FILE})
ELSE(ITK_FOUND)
	MESSAGE(FATAL_ERROR "Cannot build without ITK.  Please set ITK_DIR.")
ENDIF(ITK_FOUND)
IF(ITK_VERSION_MAJOR LESS 4)
	MESSAGE(FATAL_ERROR "Your ITK version is too old. Please use ITK >= 4.x")
ENDIF (ITK_VERSION_MAJOR LESS 4)
SET( ITK_LIBRARIES
	ITKBiasCorrection    ITKCommon       ITKDICOMParser       ITKEXPAT
	ITKIOImageBase       ITKIOBioRad     ITKIOBMP             ITKIOGDCM            ITKIOGE         ITKIOGIPL
	ITKIOHDF5            ITKIOIPL        ITKIOJPEG            ITKIOLSM             ITKIOMeta       ITKIONIFTI
	ITKIONRRD            ITKIOPNG        ITKIOSiemens         ITKIOSpatialObjects  ITKIOStimulate  ITKIOTIFF
	ITKIOVTK             ITKIOXML
	ITKKLMRegionGrowing  ITKLabelMap     ITKMesh              ITKMetaIO            ITKniftiio      ITKNrrdIO
	ITKOptimizers        ITKPath         ITKVNLInstantiation  ITKVTK               ITKVtkGlue      ITKWatersheds
	ITKznz
	itkjpeg              itkNetlibSlatec itkpng               itksys               itktiff         itkv3p_netlib
	itkvcl               itkvnl          itkvnl_algo
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
IF (ITK_VERSION_MAJOR LESS 5)
	# some libraries were removed with ITK 5:
	SET (ITK_LIBRARIES ${ITK_LIBRARIES} ITKBioCell  ITKFEM)
ENDIF()
IF (ITK_VERSION_MAJOR LESS 5 AND ITK_VERSION_MINOR LESS 12)
	# apparently, in 4.12 the itkopenjpeg.lib isn't built anymore by default
	SET (ITK_LIBRARIES ${ITK_LIBRARIES} itkopenjpeg)
ENDIF()
IF (ITK_VERSION_MAJOR GREATER 4 OR ITK_VERSION_MINOR GREATER 12)
	# starting with ITK 4.13, there is an implicit dependency on ITKIOBruker and ITKIOMINC
	SET (ITK_LIBRARIES ${ITK_LIBRARIES} ITKIOBruker ITKIOMINC)
ENDIF()
IF(ITK_VERSION_MAJOR GREATER 4 OR ITK_VERSION_MINOR GREATER 4)
	# starting with ITK 4.5, there is an implicit dependency on ITKIOMRC:
	SET(ITK_LIBRARIES ${ITK_LIBRARIES} ITKIOMRC)
	# SCIFIO only available in ITK >= 4.5?
	IF (SCIFIO_LOADED)
		ADD_DEFINITIONS(-DUSE_SCIFIO)
		MESSAGE(STATUS "ITK has SCIFIO support enabled.\n\
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
		SET(ITK_LIBRARIES ${ITK_LIBRARIES} SCIFIO)
	ENDIF(SCIFIO_LOADED)
ELSE ()
	# ITKReview apparently not required to be linked in ITK > 4.5?
	SET(ITK_LIBRARIES ${ITK_LIBRARIES} ITKReview)
ENDIF(ITK_VERSION_MAJOR GREATER 4 OR ITK_VERSION_MINOR GREATER 4)


# VTK (>= 6)
FIND_PACKAGE(VTK)
IF(VTK_FOUND)
	INCLUDE(${VTK_USE_FILE})
ELSE()
	MESSAGE(FATAL_ERROR "Cannot build without VTK. Please set VTK_DIR.")
ENDIF()
IF(VTK_VERSION_MAJOR LESS 7)
	MESSAGE(FATAL_ERROR "Your VTK version is too old. Please use VTK >= 7.0")
ENDIF()
IF ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
	MESSAGE(STATUS "VTK is using OpenGL2 Backend!")
	ADD_DEFINITIONS(-DVTK_OPENGL2_BACKEND)
ELSE()
	MESSAGE(STATUS "VTK is using OpenGL Backend.")
ENDIF()
SET (VTK_LIBRARIES
	vtkCommonCore
	vtkChartsCore
	vtkDICOMParser
	vtkFiltersCore vtkFiltersHybrid vtkFiltersModeling
	vtkGUISupportQt vtkGUISupportQtOpenGL vtkRenderingQt
	vtkImagingCore vtkImagingStatistics
	vtkInfovisCore
	vtkIOCore vtkIOMovie vtkIOGeometry vtkIOXML
	vtkRenderingCore vtkRenderingAnnotation vtkRenderingContext2D vtkRenderingFreeType vtkRenderingImage
	vtkRenderingContext${VTK_RENDERING_BACKEND} vtkRendering${VTK_RENDERING_BACKEND} vtkRenderingVolume${VTK_RENDERING_BACKEND}
	vtkViewsCore vtkViewsContext2D vtkViewsInfovis
	vtksys)
	SET (VTK_LIBRARIES ${VTK_LIBRARIES}	)
# Libraries introduced with VTK 7.1:
IF (VTK_VERSION_MAJOR GREATER 7 OR (VTK_VERSION_MAJOR EQUAL 7 AND VTK_VERSION_MINOR GREATER 0))
	SET(VTK_LIBRARIES ${VTK_LIBRARIES} vtkImagingHybrid)
	IF ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
		SET (VTK_LIBRARIES ${VTK_LIBRARIES}	vtkRenderingGL2PSOpenGL2 vtkgl2ps)
	ENDIF ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
ENDIF()
IF (vtkoggtheora_LOADED)
	MESSAGE(STATUS "Video: Ogg Theora Encoder available")
	ADD_DEFINITIONS(-DVTK_USE_OGGTHEORA_ENCODER)
ELSE()
	MESSAGE(WARNING "No video encoder available! You will not be able to record videos.")
ENDIF()


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
	ADD_DEFINITIONS(-DUSE_EIGEN)
	INCLUDE_DIRECTORIES( ${EIGEN3_INCLUDE_DIR} )
ENDIF(EIGEN3_FOUND)


# HDF5
# FIND_PACKAGE(HDF5) does not behave properly:
#     - always uses first installed version without allowing to override
#     - if not installed, reports missing HDF5_DIR and unsets it even when set to directory having same structure as install
#   => skip? for now, allow overriding with HDF5_DIR_OVERRIDE
FIND_PACKAGE(HDF5 NAMES hdf5 COMPONENTS C NO_MODULE QUIET)
IF (HDF5_DIR_OVERRIDE AND NOT "${HDF5_DIR_OVERRIDE}" STREQUAL "${HDF5_DIR}")
	SET(HDF5_DIR "${HDF5_DIR_OVERRIDE}" CACHE PATH "" FORCE)
	SET(HDF5_FOUND "true")
	MESSAGE(STATUS "HDF5: Overriding found HDF5_DIR=${HDF5_DIR} with HDF5_DIR_OVERRIDE=${HDF5_DIR_OVERRIDE}")
ENDIF()
IF (HDF5_FOUND)
	FIND_LIBRARY(HDF5_CORE_LIB hdf5 PATHS ${HDF5_DIR}/../bin ${HDF5_DIR}/../../lib ${HDF5_DIR}/../lib)
	FIND_PATH(HDF5_INCLUDE_OVERWRITE_DIR hdf5.h PATHS "${HDF5_DIR}/../include" "${HDF5_DIR}/../../include")
	SET(HDF5_INCLUDE_DIR "${HDF5_INCLUDE_OVERWRITE_DIR}" CACHE PATH "" FORCE)
	UNSET(HDF5_INCLUDE_OVERWRITE_DIR CACHE)
	IF (CMAKE_COMPILER_IS_GNUCXX)
		FIND_LIBRARY(HDF5_Z_LIB z PATHS ${HDF5_DIR}/../../lib NO_DEFAULT_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_PATH NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_FIND_ROOT_PATH)
		FIND_LIBRARY(HDF5_SZIP_LIB szip PATHS ${HDF5_DIR}/../../lib)
		SET (HDF5_LIBRARY ${HDF5_CORE_LIB} ${HDF5_SZIP_LIB} ${HDF5_Z_LIB} CACHE STRING "" FORCE)
		UNSET(HDF5_Z_LIB CACHE)
		UNSET(HDF5_SZIP_LIB CACHE)
	ELSE()
		SET (HDF5_LIBRARY ${HDF5_CORE_LIB} CACHE STRING "" FORCE)
	ENDIF()
	UNSET(HDF5_CORE_LIB CACHE)
	MESSAGE(STATUS "Found HDF5: ${HDF5_DIR}")
ENDIF()


# Astra Toolbox
FIND_PACKAGE(AstraToolbox)


# CUDA:
FIND_PACKAGE(CUDA)


# OpenCL
FIND_PACKAGE(OpenCL)


# OpenMP
INCLUDE(${CMAKE_ROOT}/Modules/FindOpenMP.cmake)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")


#-------------------------
# Library Installation
#-------------------------

# ToDo: install libraries only if required by some module!
#       Requires doing installation only after modules, and somehow setting which library is required by a module.

# ITK
SET (ITK_VER "${ITK_VERSION_MAJOR}.${ITK_VERSION_MINOR}")
IF (WIN32)
	SET (ITK_LIB_DIR "${ITK_DIR}/bin/Release")
	# strangely, under Windows, ITK seems to build a completely different set of shared libraries
	# than under Linux. Those listed below are required by our binary. Even more strangely, this
	# list is different from the list of libraries required for linking)
	SET (WIN_ITK_LIBS 
		ITKCommon	ITKIOBioRad	ITKIOBMP	ITKIOGDCM	ITKIOGE		ITKIOGIPL		ITKIOHDF5
		ITKIOImageBase	ITKIOIPL	ITKIOJPEG	ITKIOLSM	ITKIOMeta	ITKIOMRC	ITKIONIFTI
		ITKIONRRD	ITKIOPNG	ITKIOStimulate	ITKIOTIFF	ITKIOVTK	ITKIOXML	ITKTransform
	)
	IF (ITK_VERSION_MAJOR GREATER 4 OR ITK_VERSION_MINOR GREATER 10)
		# libraries introduced with ITK 4.11
		SET (WIN_ITK_LIBS ${WIN_ITK_LIBS}	ITKLabelMap	ITKOptimizers	ITKStatistics	ITKVTK	ITKWatersheds)
	ENDIF()
	IF (ITK_VERSION_MAJOR EQUAL 4 AND ITK_VERSION_MINOR EQUAL 11)
		# libraries only required under ITK 4.11
		SET (WIN_ITK_LIBS ${WIN_ITK_LIBS} ITKFFT)  
	ENDIF()
	IF (ITK_VERSION_MAJOR GREATER 4 OR ITK_VERSION_MINOR GREATER 12)
		# starting with ITK 4.13, there is an implicit dependency on ITKIOBruker and ITKIOMINC
		SET (WIN_ITK_LIBS ${WIN_ITK_LIBS} ITKIOBruker ITKIOMINC)
	ENDIF()
	IF (SCIFIO_LOADED)
		SET (WIN_ITK_LIBS ${WIN_ITK_LIBS} itkSCIFIO)
	ENDIF()
	SET (ITK_LIB_DIR "${ITK_DIR}/bin/Release")
	FOREACH(ITK_LIB ${WIN_ITK_LIBS})
		INSTALL (FILES ${ITK_LIB_DIR}/${ITK_LIB}-${ITK_VER}.dll DESTINATION .)
	ENDFOREACH(ITK_LIB)
ELSEIF (UNIX)
	SET (ITK_LIB_DIR "${ITK_DIR}/lib")
	SET (EXTRA_ITK_LIBS           ITKSpatialObjects  ITKStatistics  ITKTransform
		itkdouble-conversion  itkgdcmcharls      itkgdcmCommon  itkgdcmDICT  itkgdcmDSED  itkgdcmIOD
		itkgdcmjpeg12         itkgdcmjpeg16      itkgdcmjpeg8   itkgdcmMSFF  itkgdcmuuid  itknetlib)
	# starting with ITK 4.11, itkhdf5* libraries must not be referenced anymore, before they are required:
	IF(ITK_VERSION_MAJOR LESS 5 AND ITK_VERSION_MINOR LESS 11)
		SET(EXTRA_ITK_LIBS ${EXTRA_ITK_LIBS} itkhdf5_cpp itkhdf5)
	ENDIF()
	# They are required again only for ITK 4.12, yet here they are the only libraries without the version suffix:
	IF (ITK_VERSION_MAJOR EQUAL 4 AND ITK_VERSION_MINOR EQUAL 12)
		SET (SPECIAL_ITK_LIBS  itkhdf5_cpp itkhdf5)
		FOREACH (SPECIAL_ITK_LIB ${SPECIAL_ITK_LIBS})
			IF (EXISTS ${ITK_LIB_DIR}/lib${SPECIAL_ITK_LIB}.so.1)
				INSTALL (FILES ${ITK_LIB_DIR}/lib${SPECIAL_ITK_LIB}.so.1 DESTINATION .)
			ELSE()
				IF (EXISTS ${ITK_LIB_DIR}/lib${SPECIAL_ITK_LIB}_debug.so.1)
					INSTALL (FILES ${ITK_LIB_DIR}/lib${SPECIAL_ITK_LIB}_debug.so.1 DESTINATION .)
				ELSE()
					MESSAGE(WARNING "Library ${SPECIAL_ITK_LIB} not found!")
				ENDIF()
			ENDIF()
		ENDFOREACH()
	ENDIF()
	# previous to 4.13: libitkgdcmopenjpeg, afterwards: libitkgdcmopenjp2
	IF (ITK_VERSION_MAJOR GREATER 4 OR ITK_VERSION_MINOR GREATER 12)
		SET (EXTRA_ITK_LIBS ${EXTRA_ITK_LIBS} itkgdcmopenjp2)
	ELSE()
		SET (EXTRA_ITK_LIBS ${EXTRA_ITK_LIBS} itkgdcmopenjpeg)
	ENDIF()
	SET (ALL_ITK_LIBS ${ITK_LIBRARIES} ${EXTRA_ITK_LIBS})
	FOREACH(ITK_LIB ${ALL_ITK_LIBS})
	# hack: SCIFIO apparently needs to be linked as "SCIFIO" but the lib is called "itkSCFICIO"...
		STRING(REPLACE "SCIFIO" "itkSCIFIO" ITK_LIBF "${ITK_LIB}")
		INSTALL (FILES ${ITK_LIB_DIR}/lib${ITK_LIBF}-${ITK_VER}.so.1 DESTINATION .)
	ENDFOREACH(ITK_LIB)
ELSE()
	MESSAGE(WARNING "Installation procedure for your operating system is not yet implemented!")
ENDIF()


# VTK
SET (VTK_VER "${VTK_VERSION_MAJOR}.${VTK_VERSION_MINOR}")
SET (VTK_EXTRA_LIBS
	vtkalglib                vtkCommonColor      vtkCommonComputationalGeometry  vtkCommonDataModel
	vtkCommonExecutionModel  vtkCommonMath       vtkCommonMisc                   vtkCommonSystem
	vtkCommonTransforms      vtkexoIIc           vtkexpat                        vtkFiltersExtraction
	vtkFiltersGeneral        vtkFiltersGeometry  vtkFiltersImaging               vtkFiltersSources
	vtkFiltersStatistics     vtkFiltersTexture   vtkfreetype                     vtkhdf5
	vtkImagingColor          vtkImagingFourier   vtkImagingGeneral               vtkImagingHybrid
	vtkImagingSources        vtkInfovisLayout    vtkInteractionStyle             vtkInteractionWidgets
	vtkIOImage               vtkIOLegacy         vtkIOXMLParser                  vtkjpeg
	vtklibxml2               vtkmetaio           vtkoggtheora                    vtkpng
	vtkRenderingLabel        vtkRenderingVolume  vtktiff                         vtkverdict
	vtkViewsInfovis          vtkzlib)
IF (${VTK_MAJOR_VERSION} LESS 7 AND ${VTK_MINOR_VERSION} LESS 3)
	SET (VTK_EXTRA_LIBS ${VTK_EXTRA_LIBS}  vtkRenderingFreeTypeOpenGL)
ENDIF()
IF (${VTK_MAJOR_VERSION} GREATER 7)
	SET (VTK_EXTRA_LIBS ${VTK_EXTRA_LIBS}  vtklz4)
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
ELSEIF (UNIX)
	SET (VTK_LIB_DIR "${VTK_DIR}/lib")
	FOREACH(VTK_LIB ${VTK_ALL_LIBS})
		INSTALL (FILES ${VTK_LIB_DIR}/lib${VTK_LIB}-${VTK_VER}.so.1 DESTINATION .)
	ENDFOREACH(VTK_LIB)
	INSTALL(FILES ${VTK_LIB_DIR}/libQVTKWidgetPlugin.so DESTINATION .)
ENDIF()


# Qt
STRING(REGEX REPLACE "/lib/cmake/Qt5" "" Qt5_BASEDIR ${Qt5_DIR})
STRING(REGEX REPLACE "/cmake/Qt5" "" Qt5_BASEDIR ${Qt5_BASEDIR})	# on linux, lib is omitted if installed from package repos
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
IF(UNIX AND NOT APPLE)
	IF (EXISTS "${Qt5_BASEDIR}/lib")
		SET (QT_LIB_DIR "${Qt5_BASEDIR}/lib")
	ELSE()
		SET (QT_LIB_DIR "${Qt5_BASEDIR}")
	ENDIF()
	SET (QT_VER "${Qt5Core_VERSION}")
	SET (QT_SHORTVER "${Qt5Core_VERSION_MAJOR}")
	SET (QT_LINUX_EXTRA_LIBS Qt5DBus Qt5XcbQpa)
	SET (QT_LINUX_ALL_LIBS ${QT_ALL_LIBS} ${QT_LINUX_EXTRA_LIBS})
	FOREACH(QT_LIBCOLON ${QT_LINUX_ALL_LIBS})
		STRING (REPLACE "::" "" QT_LIB "${QT_LIBCOLON}")
		IF (EXISTS ${QT_LIB_DIR}/lib${QT_LIB}.so.${QT_VER})
			INSTALL (FILES ${QT_LIB_DIR}/lib${QT_LIB}.so.${QT_VER} DESTINATION . RENAME lib${QT_LIB}.so.${QT_SHORTVER})
		ELSE()
			MESSAGE(SEND_ERROR "Qt Installation: File ${QT_LIB_DIR}/lib${QT_LIB}.so.${QT_VER} not found!")
		ENDIF()
	ENDFOREACH(QT_LIBCOLON)

	IF (EXISTS "${QT_LIB_DIR}/libQt5X11Extras.so.${QT_VER}" )
		INSTALL (FILES ${QT_LIB_DIR}/libQt5X11Extras.so.${QT_VER} DESTINATION . RENAME libQt5X11Extras.so.${QT_SHORTVER})
	ELSE ()
		STRING(REGEX REPLACE "/qtbase" "" Qt5_BINDIR ${Qt5_BASEDIR})
		INSTALL (FILES ${Qt5_BINDIR}/qtx11extras/lib/libQt5X11Extras.so.${QT_VER} DESTINATION . RENAME libQt5X11Extras.so.${QT_SHORTVER})
	ENDIF ()

	# xcb platform plugin, and its plugins egl and glx:
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


# OpenCL
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
			MESSAGE(WARNING "OpenCL.dll was not found. You can continue building, but the program might not run (or it might fail to run when installed/cpacked).")
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


# CUDA:
IF (CUDA_FOUND)
	ADD_DEFINITIONS(-DASTRA_CUDA)
	IF (WIN32)
		INSTALL (FILES "${CUDA_TOOLKIT_ROOT_DIR}/bin/cudart64_${CUDA_VERSION_MAJOR}${CUDA_VERSION_MINOR}.dll" DESTINATION .)
		INSTALL (FILES "${CUDA_TOOLKIT_ROOT_DIR}/bin/cufft64_${CUDA_VERSION_MAJOR}${CUDA_VERSION_MINOR}.dll" DESTINATION .)
	ELSEIF(UNIX AND NOT APPLE)
		get_filename_component(CUDART_SHAREDLIB "${CUDA_CUDART_LIBRARY}" REALPATH)
		get_filename_component(CUFFT_SHAREDLIB "${CUDA_cufft_LIBRARY}" REALPATH)
		INSTALL (FILES "${CUDART_SHAREDLIB}" DESTINATION . RENAME "libcudart.so.${CUDA_VERSION}")
		INSTALL (FILES "${CUFFT_SHAREDLIB}" DESTINATION . RENAME "libcufft.so.${CUDA_VERSION}")
	ENDIF ()
ENDIF()


# ASTRA Toolbox
IF (ASTRA_TOOLBOX_FOUND)
	IF (WIN32)
		STRING (REGEX REPLACE "AstraCuda64.lib" "AstraCuda64.dll" ASTRA_RELEASE_DLL "${ASTRA_TOOLBOX_LIBRARIES_RELEASE}")
		INSTALL (FILES ${ASTRA_RELEASE_DLL} DESTINATION .)
	ELSEIF(UNIX AND NOT APPLE)
		get_filename_component(ASTRA_SHAREDLIB "${ASTRA_TOOLBOX_LIBRARIES_RELEASE}" REALPATH)
		INSTALL (FILES "${ASTRA_SHAREDLIB}" DESTINATION . RENAME libastra.so.0)
	ENDIF ()
ENDIF()


# HDF5
IF (HDF5_FOUND)
	IF (WIN32)
		SET (HDF5_LIBRARIES hdf5 szip zlib)
		STRING(REGEX REPLACE "/cmake" "" HDF5_BASE_DIR ${HDF5_DIR})
		SET (HDF5_BIN_DIR ${HDF5_BASE_DIR}/bin)
		FOREACH(HDF5_LIB ${HDF5_LIBRARIES})
			INSTALL(FILES "${HDF5_BIN_DIR}/${HDF5_LIB}.dll" DESTINATION .)
		ENDFOREACH()
	ELSE()
		STRING(REGEX REPLACE "/share/cmake" "" HDF5_BASE_DIR ${HDF5_DIR})
		SET (HDF5_LIB_DIR ${HDF5_BASE_DIR}/lib)
		# for some strange reason, hdf5 links to the .100.1.0 version...
		INSTALL(FILES "${HDF5_LIB_DIR}/libhdf5.so.${HDF5_VERSION}" RENAME libhdf5.so.100.1.0 DESTINATION .)
		INSTALL(FILES "${HDF5_LIB_DIR}/libszip.so.2.1" DESTINATION .)
		INSTALL(FILES "${HDF5_LIB_DIR}/libz.so.1.2" DESTINATION .)
	ENDIF ()
ENDIF()

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

	# sets the c++14 standard (or the next lower one which is available on the compiler)
	SET(CMAKE_CXX_STANDARD 14)
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
git_describe(VERSION --tags)
configure_file("${open_iA_SOURCE_DIR}/cmake/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)    # Enable Unicode

SET(CMAKE_INSTALL_RPATH "\$ORIGIN")      # Set RunPath in all created libraries / executables to $ORIGIN
