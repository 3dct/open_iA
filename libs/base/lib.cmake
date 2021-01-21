TARGET_LINK_LIBRARIES(${libname} PUBLIC

	Qt5::Core Qt5::Xml
	${VTK_LIB_PREFIX}CommonCore ${VTK_LIB_PREFIX}CommonDataModel ${VTK_LIB_PREFIX}CommonExecutionModel

	# ToDo: Get rid of GUI stuff here, move down to core/...
	Qt5::Gui

	# ideally, base would not reference any VTK libraries as well
	${VTK_LIB_PREFIX}GUISupportQt
	${VTK_LIB_PREFIX}IOImage
	${VTK_LIB_PREFIX}ImagingCore
	${VTK_LIB_PREFIX}RenderingCore
	${VTK_LIB_PREFIX}RenderingOpenGL2
	${VTK_LIB_PREFIX}RenderingVolumeOpenGL2
	${VTK_LIB_PREFIX}InteractionStyle      # implements VTK::RenderingCore
	${VTK_LIB_PREFIX}RenderingFreeType     # implements VTK::RenderingCore
	${VTK_LIB_PREFIX}RenderingGL2PSOpenGL2 # implements VTK::RenderingOpenGL2
	${VTK_LIB_PREFIX}RenderingOpenVR       # implements VTK::RenderingCore
	${VTK_LIB_PREFIX}RenderingUI           # implements VTK::RenderingCore

#	${ITK_LIBRARIES}
)

# instead of ITK_LIBRARIES, ideally we could write something more specific like this:
	# regarding ITKIO... dependencies to these are pulled in automatically by IO factories somehow:
SET(ITK_REQUIRED_LIBS
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
FOREACH(itklib ${ITK_REQUIRED_LIBS})
	MESSAGE(STATUS "${itklib} - lib: ${${itklib}_LIBRARIES}, include: ${${itklib}_INCLUDE_DIRS}")
	TARGET_LINK_LIBRARIES(${libname} PUBLIC ${${itklib}_LIBRARIES})
	TARGET_INCLUDE_DIRECTORIES(${libname} PUBLIC ${${itklib}_INCLUDE_DIRS})
ENDFOREACH()
