TARGET_LINK_LIBRARIES(${libname} PUBLIC iAcore)
SET( VTK_REQUIRED_LIBS_PRIVATE
	CommonComputationalGeometry # for vtkParametricSpline used in iASpline
)
IF (VTK_VERSION VERSION_LESS "9.0.0")
	LIST(APPEND VTK_REQUIRED_LIBS_PRIVATE
		InteractionWidgets     # for vtkBorderRepresentation.h, required by iARulerRepresentation.h, vtkEvent.h, required by iARulerActor.h
	)
ENDIF()
