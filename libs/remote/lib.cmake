target_link_libraries(${libname} PUBLIC Qt${QT_VERSION_MAJOR}::WebSockets)
# for VTK < 9 we have to use VTK_USE_FILE anyway for module autoinitialization
#if (VTK_VERSION VERSION_LESS "9.0.0")
#	set(VTK_REQUIRED_LIBS_PRIVATE
#		InteractionWidgets     # for vtkLogoRepresentation.h, required by iARendererImpl.cpp
#	)
#endif()


set(VTK_REQUIRED_LIBS_PUBLIC
	CommonCore
	CommonDataModel
	CommonExecutionModel
	# move to separate IO library?
	IOImage               # for volume loading; move to a new "io" library?
	IOGeometry            # for vtkSTLReader/Writer; move to a new "io" library?
	IOXML                   # VTK9, for vtkXMLImageDataReader used in iAIO
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