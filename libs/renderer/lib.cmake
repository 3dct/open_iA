target_link_libraries(${libname} PUBLIC iA::guibase)
# for VTK < 9 we have to use VTK_USE_FILE anyway for module autoinitialization
#IF (VTK_VERSION VERSION_LESS "9.0.0")
#	SET( VTK_REQUIRED_LIBS_PRIVATE
#		InteractionWidgets     # for vtkLogoRepresentation.h, required by iARendererImpl.cpp
#	)
#ENDIF()
