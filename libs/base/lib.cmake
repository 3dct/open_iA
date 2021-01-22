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
	RenderingOpenGL2
	RenderingVolumeOpenGL2
	InteractionStyle      # implements VTK::RenderingCore
	RenderingFreeType     # implements VTK::RenderingCore
	RenderingGL2PSOpenGL2 # implements VTK::RenderingOpenGL2
	RenderingOpenVR       # implements VTK::RenderingCore
	RenderingUI           # implements VTK::RenderingCore
)

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
