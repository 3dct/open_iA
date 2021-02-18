TARGET_LINK_LIBRARIES(${libname} PUBLIC iAcore)
IF (VTK_VERSION VERSION_LESS "9.0.0")
	SET( VTK_REQUIRED_LIBS_PRIVATE
		InteractionWidgets     # for vtkLogoRepresentation.h, required by iARendererImpl.cpp
	)
ENDIF()
