SET( MODULE_DESCRIPTION_OUT
	"Intensity filters:\nDifference\nSubtractImages Filter\nInvertIntensity\nMask Image\nIntensity Windowing\nNormalize Image\nHistogram Match"
	PARENT_SCOPE
)
SET( MODULE_DEFAULT_OPTION_VALUE_OUT OFF  PARENT_SCOPE)

IF (openiA_TESTING_ENABLED AND Module_Intensity)
	ADD_TEST(NAME CMD_Invert COMMAND ${TEST_CMD_Binary} -r "Invert" -i ${TEST_DATA_DIR}/test2x2x2.mhd -o ${CMAKE_BINARY_DIR}/Testing/Temporary/test_invert.mhd -p true 1 -q -f)
	# TODO: check output?
ENDIF()