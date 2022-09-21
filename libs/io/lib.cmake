target_link_libraries(${libname} PUBLIC
	iA::base
)
set(VTK_REQUIRED_LIBS_PUBLIC
	# move to separate IO library?
	IOImage               # for volume loading; move to a new "io" library?
	IOGeometry            # for vtkSTLReader/Writer; move to a new "io" library?
	IOXML                   # VTK9, for vtkXMLImageDataReader used in iAIO
)
if (TARGET VTK::RenderingOpenVR)
	list(APPEND VTK_REQUIRED_LIBS_PUBLIC
		RenderingOpenVR)       # implements VTK::RenderingCore
endif()

if (HDF5_FOUND)
	# as HDF5 is required only here, we could link privately, but under Linux this leads
	# to gui and cmd also requiring linking to it separately, it's easier to link PUBLIC here:
	target_link_libraries(${libname} PUBLIC hdf5::hdf5-static)
	target_compile_definitions(${libname} PUBLIC USE_HDF5)	# must be public because used in header defines.h
endif()