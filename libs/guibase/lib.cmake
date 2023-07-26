target_link_libraries(${libname} PUBLIC
	iA::base
	iA::charts   # for iAImageDataForDisplay
	iA::qthelper # for iAImageDataForDisplay
	iA::io       # for iAOIFReader in iAIO - can be removed once iAIO isn't there anymore
	Qt::Concurrent
)
set(VTK_REQUIRED_LIBS_PUBLIC
	FiltersGeometry         # for vtkImageDataGeometryFilter, used in iALabel3D and vtkDataSetSurfaceFilter used in ExtractSurface - iAExtractSurfaceFilter
	FiltersModeling
	IOGeometry              # for vtkSTLReader/Writer
	IOMovie                 # for vtkAVIWriter, vtkGenericMovieWriter
	IOOggTheora             # for vtkOggTheoraWriter
	IOXML                   # for vtkXMLImageDataReader used in iAIO
	RenderingAnnotation     # for vtkAnnotatedCubeActor, vtkCaptionActor, vtkScalarBarActor
	RenderingQt             # for vtkQImageToImageSource, also pulls in vtkGUISupportQt (for QVTKWidgetOpenGL)
)
#set(ITK_REQUIRED_LIBS_PUBLIC
#	ITKIORAW                # for RawImage... in iAIO
#)
#if (ITK_VERSION VERSION_LESS "5.0.0")
#	LIST(APPEND ITK_REQUIRED_LIBS_PUBLIC
#		ITKGDCM         # for gdcmSerieHelper.h, required by itkGDCMSeriesFileNames.h (via iAIO)
#	)
#endif()
if ("${VTK_VIDEO_SUPPORT}" STREQUAL "ogg")
	target_compile_definitions(${libname} PRIVATE VTK_USE_OGGTHEORA_ENCODER)
endif()
if (VTK_USE_AVIWRITER)
	target_compile_definitions(${libname} PRIVATE VTK_USE_AVIWRITER)
endif()
if (SCIFIO_LOADED)
	target_compile_definitions(${libname} PRIVATE USE_SCIFIO)
endif()
if (OpenMP_CXX_FOUND)
	target_link_libraries(${libname} PUBLIC OpenMP::OpenMP_CXX)
	if (MSVC)
		target_compile_options(${libname} PUBLIC /Zc:twoPhase-)
	endif()
endif()
if (openiA_CHART_OPENGL)
	target_compile_definitions(${libname} PUBLIC CHART_OPENGL)
endif()
if (MSVC)
	target_compile_options(${libname} PRIVATE "/bigobj")
endif()
