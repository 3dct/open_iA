set(SPECTRA_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/InSpectr/refSpectra.7z")
add_custom_command(TARGET InSpectr POST_BUILD COMMAND ${CMAKE_COMMAND}
		"-DSPECTRA_ARCHIVE=${SPECTRA_ARCHIVE}" "-DTARGET_DIR=$<TARGET_FILE_DIR:iA::guibase>/refSpectra"
		-P "${CMAKE_CURRENT_SOURCE_DIR}/InSpectr/extractSpectra.cmake"
)

install(CODE "
	execute_process(COMMAND ${CMAKE_COMMAND}
		\"-DSPECTRA_ARCHIVE=${SPECTRA_ARCHIVE}\" \"-DTARGET_DIR=\${CMAKE_INSTALL_PREFIX}/refSpectra\"
		-P \"${CMAKE_CURRENT_SOURCE_DIR}/InSpectr/extractSpectra.cmake\"
	)
")
