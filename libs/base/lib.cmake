target_link_libraries(${libname} PUBLIC
	${ITK_LIBRARIES}
	Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Xml
	# ToDo: Get rid of GUI stuff here, move down to core/...
	Qt${QT_VERSION_MAJOR}::Gui
	# ::Widgets # seems to be pulled in by vtk's GUISupportQt automatically
)
if (VTK_VERSION VERSION_LESS "9.0.0")
	target_link_libraries(${libname} PUBLIC ${VTK_LIBRARIES})
else()
	set(VTK_REQUIRED_LIBS_PUBLIC
		CommonCore
		CommonDataModel
		CommonExecutionModel
		IOImage               # for volume loading; move to a new "io" library?
		IOGeometry            # for vtkSTLReader/Writer; move to a new "io" library?
		# ideally, base would not reference any VTK libraries;
		# at least the following GUI/Rendering library references should be removed:
		GUISupportQt
		ImagingCore
		RenderingCore
		RenderingOpenGL2
		RenderingVolumeOpenGL2
		InteractionStyle      # implements VTK::RenderingCore
		RenderingFreeType     # implements VTK::RenderingCore
		RenderingGL2PSOpenGL2 # implements VTK::RenderingOpenGL2
		RenderingUI           # implements VTK::RenderingCore
	)
	# for VTK < 9 we have to use VTK_USE_FILE anyway for module autoinitialization
	#	LIST(APPEND VTK_REQUIRED_LIBS_PUBLIC
	#		CommonMath          # for vtkTuple.h, required by Common/DataModel/vtkVector.h
	#		FiltersCore         # for vtkFiltersCoreModule.h, required by vtkRenderingCoreModule.h
	#		GUISupportQtOpenGL  # for QVTKWidget2.h
	#		RenderingVolume     # for vtkRenderingVolumeModule.h, required by vtkRenderingVolumeOpenGLModule.h
	#	)

	if (TARGET VTK::RenderingOpenVR)
		list(APPEND VTK_REQUIRED_LIBS_PUBLIC
			RenderingOpenVR)       # implements VTK::RenderingCore
	endif()
endif()
# VTK_REQUIRED_LIBS_PUBLIC above only used anyway on VTK >= 9, which only supports OpenGL2 backend...
#	list(APPEND VTK_REQUIRED_LIBS_PUBLIC
#		RenderingGL2PSOpenGL2 # implements VTK::RenderingOpenGL2
#	)

#	
# instead of linking all ITK_LIBRARIES:
	# regarding ITKIO... dependencies to these are pulled in automatically by IO factories somehow:
#set(ITK_REQUIRED_LIBS_PUBLIC
#	ITKCommon
#	ITKEigen3           # drawn in by some core math
#	ITKImageFilterBase  # for e.g. CastImageFilter
#	ITKImageFunction    # dependency of ITKImageGrid (even though not of all filters in it ...)
#	ITKImageGrid        # for e.g. ChangeInformationImageFilter
#	ITKImageIntensity   # for e.g. RescaleIntensityImageFilter
#	ITKImageStatistics  # for e.g. StatisticsImageFilter
#	ITKIOBMP
#	ITKIOBioRad
#	ITKIOBMP
#	ITKIOBruker
#	ITKIOGDCM
#	ITKIOGE
#	ITKIOGIPL
#	ITKIOHDF5
#	ITKIOImageBase
#	ITKIOJPEG
#	ITKIOJPEG2000
#	ITKIOLSM
#	ITKIOMeta
#	ITKIOMINC
#	ITKIOMRC
#	ITKIONIFTI
#	ITKIONRRD
#	ITKIOPNG
#	ITKIOTIFF
#	ITKIOVTK
#	ITKVNL             # drawn in by some core math
#	ITKVTK
#)
## version check needs to be verified, not sure if these dependencies were introduced exactly with v5.0.0
## currently known: they are required in ITK 4.10.0, but not in 5.1.0
#if (ITK_VERSION VERSION_LESS "5.0.0")
#	list(APPEND ITK_REQUIRED_LIBS_PUBLIC ITKKWSys)
#endif()
