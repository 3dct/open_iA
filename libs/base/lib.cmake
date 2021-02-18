TARGET_LINK_LIBRARIES(${libname} PUBLIC
	Qt5::Core Qt5::Xml
	# ToDo: Get rid of GUI stuff here, move down to core/...
	Qt5::Gui
)
SET (VTK_REQUIRED_LIBS_PUBLIC
	CommonCore
	CommonDataModel
	CommonExecutionModel
	# ideally, base would not reference any VTK libraries, but at least not GUI/Rendering libraries:
	GUISupportQt
	IOImage
	ImagingCore
	RenderingCore
	Rendering${VTK_RENDERING_BACKEND}
	RenderingVolume${VTK_RENDERING_BACKEND}
	InteractionStyle      # implements VTK::RenderingCore
	RenderingFreeType     # implements VTK::RenderingCore
	RenderingOpenVR       # implements VTK::RenderingCore
	RenderingUI           # implements VTK::RenderingCore
)
IF (VTK_VERSION VERSION_LESS "9.0.0")
	LIST(APPEND VTK_REQUIRED_LIBS_PUBLIC
		CommonMath          # for vtkTuple.h, required by Common/DataModel/vtkVector.h
		FiltersCore         # for vtkFiltersCoreModule.h, required by vtkRenderingCoreModule.h
		GUISupportQtOpenGL  # for QVTKWidget2.h
		RenderingVolume     # for vtkRenderingVolumeModule.h, required by vtkRenderingVolumeOpenGLModule.h
	)
ENDIF()
IF ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
	LIST(APPEND VTK_REQUIRED_LIBS_PUBLIC
		RenderingGL2PSOpenGL2 # implements VTK::RenderingOpenGL2
	)
ENDIF()

#	${ITK_LIBRARIES}
# instead of linking all ITK_LIBRARIES:
	# regarding ITKIO... dependencies to these are pulled in automatically by IO factories somehow:
SET(ITK_REQUIRED_LIBS_PUBLIC
	ITKCommon
	ITKEigen3           # drawn in by some core math
	ITKImageFilterBase  # for e.g. CastImageFilter
	ITKImageFunction    # dependency of ITKImageGrid (even though not of all filters in it ...)
	ITKImageGrid        # for e.g. ChangeInformationImageFilter
	ITKImageIntensity   # for e.g. RescaleIntensityImageFilter
	ITKImageStatistics  # for e.g. StatisticsImageFilter
	ITKIOBMP
	ITKIOBioRad
	ITKIOBMP
	ITKIOBruker
	ITKIOGDCM
	ITKIOGE
	ITKIOGIPL
	ITKIOHDF5
	ITKIOImageBase
	ITKIOJPEG
	ITKIOJPEG2000
	ITKIOLSM
	ITKIOMeta
	ITKIOMINC
	ITKIOMRC
	ITKIONIFTI
	ITKIONRRD
	ITKIOPNG
	ITKIOTIFF
	ITKIOVTK
	ITKVNL             # drawn in by some core math
	ITKVTK
)
# version check needs to be verified, not sure if these dependencies were introduced exactly with v5.0.0
# currently known: they are required in ITK 4.10.0, but not in 5.1.0
IF (ITK_VERSION VERSION_LESS "5.0.0")
	LIST(APPEND ITK_REQUIRED_LIBS_PUBLIC ITKKWSys)
ENDIF()
