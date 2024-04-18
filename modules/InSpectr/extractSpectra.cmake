if (NOT EXISTS "${TARGET_DIR}" OR "${SPECTRA_ARCHIVE}" IS_NEWER_THAN "${TARGET_DIR}")
	message("Extracting ${SPECTRA_ARCHIVE} into target directory ${TARGET_DIR}.")
	execute_process(
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${TARGET_DIR}"
		COMMAND "${CMAKE_COMMAND}" -E chdir "${TARGET_DIR}" "${CMAKE_COMMAND}" -E tar "xf" "${SPECTRA_ARCHIVE}"
		RESULT_VARIABLE res
		OUTPUT_VARIABLE out)
	if (NOT "${res}" EQUAL "0")
		message(SEND_ERROR "  Extracting ${SPECTRA_ARCHIVE} to ${TARGET_DIR} FAILED! exit code=${res}, output: ${out}")
	endif()
else()
	message("Target directory ${TARGET_DIR} already exists, will not extract ${SPECTRA_ARCHIVE}.")
endif()
