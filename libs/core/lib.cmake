TARGET_LINK_LIBRARIES(${libname} PUBLIC
	Qt5::Concurrent
	iAbase
)
SET(VTK_REQUIRED_LIBS_PUBLIC
	FiltersGeometry         # VTK9, for vtkImageDataGeometryFilter, used in iALabel3D and vtkDataSetSurfaceFilter used in ExtractSurface - iAExtractSurfaceFilter
	FiltersModeling
	IOGeometry              # for vtkSTLReader/Writer
	IOMovie                 # for vtkAVIWriter, vtkGenericMovieWriter
	IOOggTheora             # for vtkOggTheoraWriter
	IOXML                   # VTK9, for vtkXMLImageDataReader used in iAIO
	RenderingAnnotation     # for vtkAnnotatedCubeActor, vtkCaptionActor, vtkScalarBarActor
	RenderingQt             # for vtkQImageToImageSource, also pulls in vtkGUISupportQt (for QVTKWidgetOpenGL)
)
SET(ITK_REQUIRED_LIBS_PUBLIC
	ITKIORAW                # for RawImage... in iAIO
)
IF ("${VTK_VIDEO_SUPPORT}" STREQUAL "ogg")
	TARGET_COMPILE_DEFINITIONS(${libname} PRIVATE VTK_USE_OGGTHEORA_ENCODER)
ENDIF()
IF (HDF5_FOUND)
	# as HDF5 is required only in core, we could link privately, but under Linux this leads
	# to gui and cmd also requiring linking to it separately, it's easier to link PUBLIC here:
	TARGET_LINK_LIBRARIES(${libname} PUBLIC ${HDF5_LIBRARY})
	# make sure HDF5 is included before itk (which brings its own hdf5 libraries in a different version):
	TARGET_INCLUDE_DIRECTORIES(${libname} BEFORE PRIVATE ${HDF5_INCLUDE_DIR})
	TARGET_COMPILE_DEFINITIONS(${libname} PRIVATE USE_HDF5)
ENDIF()
IF (SCIFIO_LOADED)
	TARGET_COMPILE_DEFINITIONS(${libname} PRIVATE USE_SCIFIO)
ENDIF()
IF (WIN32)
	# apparently required for VS 2015, and doesn't hurt for VS2013:
	TARGET_LINK_LIBRARIES(${libname} PUBLIC Opengl32)
ENDIF (WIN32)
IF(OpenMP_CXX_FOUND)
	TARGET_LINK_LIBRARIES(${libname} PUBLIC OpenMP::OpenMP_CXX)
	IF (MSVC)
		TARGET_COMPILE_OPTIONS(${libname} PUBLIC /Zc:twoPhase-)
	ENDIF()
ENDIF()
if (openiA_CHART_OPENGL)
	TARGET_COMPILE_DEFINITIONS(${libname} PUBLIC CHART_OPENGL)
endif()
IF (MSVC)
	TARGET_COMPILE_OPTIONS(${libname} PRIVATE "/bigobj")
ENDIF()