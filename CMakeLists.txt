CMAKE_MINIMUM_REQUIRED ( VERSION 3.0 )

PROJECT ( open_iA LANGUAGES CXX  )

INCLUDE (CTest)
enable_testing()

# Enable Unicode
ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

#-----------------------------------------------------------------------------------------
# PACKAGES
#-----------------------------------------------------------------------------------------


# ITK
FIND_PACKAGE(ITK)
IF(ITK_FOUND)
	INCLUDE(${ITK_USE_FILE})
	INCLUDE_DIRECTORIES($ENV{ITK_APPS_SRC_PATH}/QtITK )
	set_target_properties(${ITK} PROPERTIES MAP_INPORTED_CONFIG_RELWITHDEBUGINFO RELEASE)
	get_cmake_property(_variableNames VARIABLES)
ELSE(ITK_FOUND)
	MESSAGE(FATAL_ERROR "Cannot build without ITK.  Please set ITK_DIR.")
ENDIF(ITK_FOUND)


# VTK
FIND_PACKAGE(VTK)

FIND_PACKAGE(VTK)
IF(VTK_FOUND)
	INCLUDE(${VTK_USE_FILE})
	include_directories(${VTK_INCLUDE_DIRS})
	set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS ${VTK_DEFINITIONS})
	set_target_properties(${VTK} PROPERTIES MAP_INPORTED_CONFIG_RELWITHDEBUGINFO RELEASE)
ELSE(VTK_FOUND)
	MESSAGE(FATAL_ERROR "Cannot build without VTK.  Please set VTK_DIR.")
ENDIF(VTK_FOUND)


# Qt
SET(CMAKE_AUTOMOC ON)

SET(QT_USE_QTXML TRUE)
IF (WIN32)
	SET( CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "C:/Program Files (x86)/Windows Kits/8.1/Lib/winv6.3/um/x64" )
ENDIF (WIN32)

FIND_PACKAGE(Qt5 COMPONENTS Widgets Xml Network Test OpenGL PrintSupport REQUIRED)
# Qt5OpenGL_INCLUDE_DIRS seems to be required on linux only, but doesn't hurt on Windows:
INCLUDE_DIRECTORIES(${Qt5Widgets_INCLUDE_DIRS} ${Qt5OpenGL_INCLUDE_DIRS} )

SET(QT_LIBRARIES ${Qt5Core_LIBRARIES} ${Qt5Xml_LIBRARIES} ${Qt5OpenGL_LIBRARIES} ${Qt5Network_LIBRARIES} ${Qt5PrintSupport_LIBRARIES})

IF (CMAKE_COMPILER_IS_GNUCXX)
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIE")
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
ENDIF()

SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/Modules")


# Eigen
FIND_PACKAGE(Eigen3)
IF(EIGEN3_FOUND)
	ADD_DEFINITIONS( -DUSE_EIGEN )
	INCLUDE_DIRECTORIES( ${EIGEN3_INCLUDE_DIR} )
ENDIF(EIGEN3_FOUND)


#OPENCL selection
option (open_iA_GPU_USING_OPENCL "" OFF)

IF(open_iA_GPU_USING_OPENCL)
	INCLUDE(${PROJECT_SOURCE_DIR}/cmake/Modules/FindOpenCL.cmake)

	IF(OPENCL_FOUND)
		INCLUDE_DIRECTORIES( ${OPENCL_INCLUDE_DIRS} )
	ELSE(OPENCL_FOUND)
	#try again with modified ITK-version of FindOpenCL.cmake
		INCLUDE(${PROJECT_SOURCE_DIR}/cmake/Modules/FindOpenCL2.cmake)
	ELSE(OPENCL_FOUND)
		MESSAGE(SEND_ERROR "OpenCL is not found. Please check your configuration.")
	ENDIF(OPENCL_FOUND)
ENDIF(open_iA_GPU_USING_OPENCL)


#OpenMP
INCLUDE(${CMAKE_ROOT}/Modules/FindOpenMP.cmake)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")

if (CMAKE_COMPILER_IS_GNUCXX)
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
	CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

	IF (COMPILER_SUPPORTS_CXX11)
		SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	ELSEIF (COMPILER_SUPPORTS_CXX0X)
		SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
	ELSE ()
		message(WARN "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
	ENDIF(COMPILER_SUPPORTS_CXX11)

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
		MESSAGE (STATUS "You are using Apple.")
		#set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
		#MESSAGE (STATUS "${CMAKE_C_FLAGS}")
	ENDIF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
	SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

ENDIF (CMAKE_COMPILER_IS_GNUCXX)

IF (MSVC)
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/x64/Debug")
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/x64/Release")
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/x64/RelWithDebInfo")
	SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/x64/MinSizeRel")

	IF (NOT EXISTS ${PROJECT_BINARY_DIR}/ALL_BUILD.vcxproj.user)
		CONFIGURE_FILE( ${PROJECT_SOURCE_DIR}/core/ALL_BUILD.vcxproj.user ${PROJECT_BINARY_DIR}/ALL_BUILD.vcxproj.user )
	ENDIF (NOT EXISTS ${PROJECT_BINARY_DIR}/ALL_BUILD.vcxproj.user)
ENDIF(MSVC)

SET (CORE_LIBRARY_NAME open_iA_Core)


#========================================================================================
# collect required libraries
#========================================================================================

# vtk libraries
IF (VTK_VERSION_MAJOR GREATER 5)
	IF ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
		SET( VTK_LIBRARIES
			vtkCommonCore
			vtkChartsCore
			vtkDICOMParser
			vtkFiltersCore
			vtkGUISupportQt
			vtkGUISupportQtOpenGL
			vtkImagingCore
			vtkInfovisCore
			vtkIOCore
			vtkIOMovie
			vtkIOGeometry
			vtkImagingStatistics
			vtkIOGeometry
			vtkIOMovie
			vtkIOXML
			vtkRenderingContext2D
			vtkRenderingContextOpenGL2
			vtkRenderingCore
			vtkRenderingFreeType
			vtkRenderingImage
			vtkRenderingOpenGL2
			vtkRenderingQt
			vtkRenderingVolumeOpenGL2
			vtkViewsCore
			vtkViewsContext2D
			vtkViewsInfovis
			vtksys
		)
	ELSE ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
		SET( VTK_LIBRARIES
			vtkChartsCore
			vtkCommonCore
			vtkDICOMParser
			vtkFiltersCore
			vtkGUISupportQt
			vtkGUISupportQtOpenGL
			vtkImagingCore
			vtkImagingStatistics
			vtkInfovisCore
			vtkIOCore
			vtkIOMovie
			vtkIOXML
			vtkIOGeometry
			vtkRenderingImage
			vtkRenderingOpenGL
			vtksys
			vtkViewsCore
			vtkViewsInfovis
		)
		IF (VTK_VERSION_MAJOR GREATER 6 OR  VTK_VERSION_MINOR GREATER 0)
			SET (VTK_LIBRARIES ${VTK_LIBRARIES}
				vtkRenderingCore
				vtkRenderingFreeType
				vtkRenderingQt
				vtkRenderingVolumeOpenGL
				vtkViewsContext2D
			)
			IF (VTK_VERSION_MAJOR GREATER 6 OR VTK_VERSION_MINOR GREATER 1)
				SET (VTK_LIBRARIES ${VTK_LIBRARIES}
					vtkRenderingContextOpenGL
				)
			ENDIF (VTK_VERSION_MAJOR GREATER 6 OR VTK_VERSION_MINOR GREATER 1)
		ENDIF (VTK_VERSION_MAJOR GREATER 6 OR  VTK_VERSION_MINOR GREATER 0)
	ENDIF("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
	
ELSE (VTK_VERSION_MAJOR GREATER 5)
	STATUS(SEND_ERROR "Your VTK version is too old. Please use VTK >= 6.0")
ENDIF (VTK_VERSION_MAJOR GREATER 5)

IF (ITK_VERSION_MAJOR GREATER 3)
	SET( ITK_LIBRARIES
		ITKBiasCorrection			ITKBioCell				ITKCommon				ITKIOImageBase
		ITKFEM						ITKIOBioRad				ITKIOBMP				ITKIOGDCM				ITKIOGE
		ITKIOGIPL					ITKIOHDF5				ITKIOIPL				ITKIOJPEG				ITKIOLSM
		ITKIOMeta					ITKIONIFTI				ITKIONRRD				ITKIOPNG				ITKIOSiemens
		ITKIOSpatialObjects			ITKIOStimulate			ITKIOTIFF				ITKIOVTK				ITKIOXML
		ITKVtkGlue					ITKKLMRegionGrowing		ITKMesh					ITKOptimizers			ITKPath
		ITKVNLInstantiation			ITKVTK					ITKWatersheds			ITKDICOMParser			ITKEXPAT
		ITKLabelMap

		itkhdf5						itkhdf5_cpp				itkjpeg					ITKMetaIO				itkNetlibSlatec
		ITKniftiio					ITKNrrdIO				itkopenjpeg				itkpng					itksys
		itktiff						itkv3p_lsqr				itkv3p_netlib			itkvcl					itkvnl
		itkvnl_algo					itkzlib					ITKznz
	)
	IF (NOT ${ITKGPUCommon_LIBRARY_DIRS} STREQUAL "")
		# cannot use ITKGPUCommon_LOADED - it is always defined - bug?
		SET( ITK_LIBRARIES  ${ITK_LIBRARIES}
			ITKGPUAnisotropicSmoothing	ITKGPUCommon		ITKGPUFiniteDifference  ITKGPUImageFilterBase
			ITKGPUSmoothing 		ITKGPUThresholding
		)
	ELSE(ITKGPUCommon_LOADED)
		IF (open_iA_GPU_USING_OPENCL)
			MESSAGE(SEND_ERROR "You're trying to build with GPU support (open_iA_GPU_USING_OPENCL), but your ITK build has GPU support disabled! Please build ITK libraries with ITK_USE_GPU enabled!")
		ENDIF()
	ENDIF()

	IF (ITK_VERSION_MAJOR GREATER 4 OR (ITK_VERSION_MAJOR EQUAL 4 AND ITK_VERSION_MINOR GREATER 4) )
		# starting with ITK 4.5, Review has a dependency on ITKIOMRC it seems:
		SET(ITK_LIBRARIES ${ITK_LIBRARIES} ITKIOMRC)
		IF (SCIFIO_LOADED)
			ADD_DEFINITIONS( -DUSE_SCIFIO )
			MESSAGE(STATUS "ITK has SCIFIO support enabled. Notice that in order to run a build with this library on another machine than the one you built it, the environment variable SCIFIO_PATH has to be set to the path containing the SCIFIO jar files! Otherwise loading images will fail!")
			SET (SOURCEDIR "${ITK_DIR}/lib/jars")
			IF (MSVC)
				SET (DESTDIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}/scifio_jars")
				configure_file("${SOURCEDIR}/bioformats_package.jar" "${DESTDIR}/bioformats_package.jar" COPYONLY)
				configure_file("${SOURCEDIR}/scifio-itk-bridge.jar" "${DESTDIR}/scifio-itk-bridge.jar" COPYONLY)
				SET (DESTDIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/scifio_jars")
				configure_file("${SOURCEDIR}/bioformats_package.jar" "${DESTDIR}/bioformats_package.jar" COPYONLY)
				configure_file("${SOURCEDIR}/scifio-itk-bridge.jar" "${DESTDIR}/scifio-itk-bridge.jar" COPYONLY)
				SET (DESTDIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL}/scifio_jars")
				configure_file("${SOURCEDIR}/bioformats_package.jar" "${DESTDIR}/bioformats_package.jar" COPYONLY)
				configure_file("${SOURCEDIR}/scifio-itk-bridge.jar" "${DESTDIR}/scifio-itk-bridge.jar" COPYONLY)
				SET (DESTDIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO}/scifio_jars")
				configure_file("${SOURCEDIR}/bioformats_package.jar" "${DESTDIR}/bioformats_package.jar" COPYONLY)
				configure_file("${SOURCEDIR}/scifio-itk-bridge.jar" "${DESTDIR}/scifio-itk-bridge.jar" COPYONLY)
			ELSE()
				SET (DESTDIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/scifio_jars")
				configure_file("${SOURCEDIR}/bioformats_package.jar" "${DESTDIR}/bioformats_package.jar" COPYONLY)
				configure_file("${SOURCEDIR}/scifio-itk-bridge.jar" "${DESTDIR}/scifio-itk-bridge.jar" COPYONLY)
			ENDIF()
			SET(ITK_LIBRARIES ${ITK_LIBRARIES} SCIFIO)
		ENDIF()
	
	ENDIF()
ELSE (ITK_VERSION_MAJOR GREATER 3)
	MESSAGE(SEND_ERROR "Your ITK version is too old. Please use ITK >= 4.x")
ENDIF (ITK_VERSION_MAJOR GREATER 3)

SET(Toolkit_DIR "${PROJECT_SOURCE_DIR}/Toolkit")

ADD_SUBDIRECTORY(Toolkit)
ADD_SUBDIRECTORY(core)
ADD_SUBDIRECTORY(modules)
ADD_SUBDIRECTORY(gui)

# INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR}/Toolkit )

# Configure the default TESTING_DATA_ROOT for the location of Data.
set(TESTING_DATA_OUTPUT ${PROJECT_SOURCE_DIR}/Test_files/TestITK/Output)

#ITK testing
option (open_iA_TEST_ITK "" OFF)
if(open_iA_TEST_ITK)
	add_subdirectory("${PROJECT_SOURCE_DIR}/Test_files/TestITK")
endif(open_iA_TEST_ITK)
