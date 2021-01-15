TARGET_LINK_LIBRARIES(${libname} PUBLIC

	Qt5::Core Qt5::Xml
	${VTK_LIB_PREFIX}CommonCore ${VTK_LIB_PREFIX}CommonDataModel ${VTK_LIB_PREFIX}CommonExecutionModel

	# ToDo: Get rid of GUI stuff here, move down to core/...
	Qt5::Gui

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

	${ITK_LIBRARIES}
)
