TARGET_LINK_LIBRARIES(${libname} PUBLIC

	Qt5::Core Qt5::Xml
	VTK::CommonCore VTK::CommonDataModel VTK::CommonExecutionModel

	# ToDo: Get rid of GUI stuff here, move down to core/...
	Qt5::Gui

	VTK::GUISupportQt
	VTK::IOImage VTK::ImagingCore
	VTK::RenderingCore VTK::RenderingOpenGL2 VTK::RenderingVolumeOpenGL2
	VTK::InteractionStyle      # implements VTK::RenderingCore
	VTK::RenderingFreeType     # implements VTK::RenderingCore
	VTK::RenderingGL2PSOpenGL2 # implements VTK::RenderingOpenGL2
	VTK::RenderingOpenVR       # implements VTK::RenderingCore
	VTK::RenderingUI           # implements VTK::RenderingCore

	${ITK_LIBRARIES}
)
