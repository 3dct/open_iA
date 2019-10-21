SET( MODULE_DESCRIPTION_OUT
	"Use XRF visualization:\nMulti-modal X-Ray Fluorescence visualization"
	PARENT_SCOPE
)
SET( MODULE_DEFAULT_OPTION_VALUE_OUT OFF  PARENT_SCOPE)

INSTALL(DIRECTORY DESTINATION refSpectra)
INSTALL(CODE "
    SET (SPECTRA_ARCHIVE \"${CMAKE_CURRENT_SOURCE_DIR}/refSpectra.7z\")
    MESSAGE(\"Extracting \${SPECTRA_ARCHIVE} in \${CMAKE_INSTALL_PREFIX}/refSpectra/...\")
	EXECUTE_PROCESS(
		COMMAND           \"\${CMAKE_COMMAND}\" -E tar \"xf\" \"\${SPECTRA_ARCHIVE}\"
		WORKING_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/refSpectra/\"
		RESULT_VARIABLE   res
		OUTPUT_VARIABLE   out)
	IF (NOT \"\${res}\" EQUAL \"0\")
		MESSAGE(SEND_ERROR \"...FAILED! exit code=\${res}, output: \${out}\")
	ELSE()
		MESSAGE(\"...Success!\")
	ENDIF()
")
