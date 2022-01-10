target_link_libraries(${libname} PUBLIC
	iA::base
	Qt${QT_VERSION_MAJOR}::Concurrent
)
set(VTK_REQUIRED_LIBS_PUBLIC
	FiltersGeometry         # VTK9, for vtkImageDataGeometryFilter, used in iALabel3D and vtkDataSetSurfaceFilter used in ExtractSurface - iAExtractSurfaceFilter
	FiltersModeling
	IOGeometry              # for vtkSTLReader/Writer
	IOMovie                 # for vtkAVIWriter, vtkGenericMovieWriter
	IOOggTheora             # for vtkOggTheoraWriter
	IOXML                   # VTK9, for vtkXMLImageDataReader used in iAIO
	RenderingAnnotation     # for vtkAnnotatedCubeActor, vtkCaptionActor, vtkScalarBarActor
	RenderingQt             # for vtkQImageToImageSource, also pulls in vtkGUISupportQt (for QVTKWidgetOpenGL)
)
# for VTK < 9 we have to use VTK_USE_FILE anyway for module autoinitialization
#if (VTK_VERSION VERSION_LESS "9.0.0")
#	LIST(APPEND VTK_REQUIRED_LIBS_PUBLIC
#		CommonMisc             # for vtkContourValues.h, required by vtkMarchingContourFilter.h
#		CommonTransforms       # for vtkTransform.h, required by iAChannel[Slicer]Data.cpp
#		FiltersGeneral         # for vtkFiltersGeneralModule.h, required by vtkFiltersModelingModule.h
#		FiltersSources         # for vtkLineSource.h, required by iALabel3D.cpp
#		IOCore                 # for vtkAbstractPolyDataReader.h, required by vtkSTLReader.h (iAIO)
#		IOLegacy               # for vtkGenericDataObjectReader.h, required by iAIO.cpp
#		RenderingLabel         # for vtkRenderingLabelModule.h, required by vtkRenderingQtModule.h (iALabel3D)
#		InteractionWidgets     # for vtkLogoRepresentation.h, required by iARendererImpl.cpp
#	)
#endif()
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
if (HDF5_FOUND)
	# as HDF5 is required only in core, we could link privately, but under Linux this leads
	# to gui and cmd also requiring linking to it separately, it's easier to link PUBLIC here:
	target_link_libraries(${libname} PUBLIC ${HDF5_LIBRARY})
	# make sure HDF5 is included before itk (which brings its own hdf5 libraries in a different version):
	target_include_directories(${libname} BEFORE PRIVATE ${HDF5_INCLUDE_DIR})
	target_compile_definitions(${libname} PUBLIC USE_HDF5)	# must be public because used in header defines.h
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
