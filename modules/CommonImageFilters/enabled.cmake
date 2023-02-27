if (openiA_TESTING_ENABLED)
	add_test(NAME CMD_DatatypeConversion1 COMMAND ${TEST_CMD_Binary} -r "Datatype Conversion" -i ${TEST_DATA_DIR}/test2x2x2.mhd -o ${CMAKE_BINARY_DIR}/Testing/Temporary/test_datatypeconv1.mhd -p "32 bit floating point number (7 digits, float)" true true 0 0 false 0 1 -q -f)

	add_test(NAME CMD_DatatypeConversion2 COMMAND ${TEST_CMD_Binary} -r "Datatype Conversion" -i ${TEST_DATA_DIR}/test2x2x2.mhd -o ${CMAKE_BINARY_DIR}/Testing/Temporary/test_datatypeconv2.mhd -p "8 bit unsigned integer (0 to 255, unsigned char)" true true 0 0 true 0 0 -q -f)

	add_test(NAME CMD_Invert COMMAND ${TEST_CMD_Binary} -r "Invert" -i ${TEST_DATA_DIR}/test2x2x2.mhd -o ${CMAKE_BINARY_DIR}/Testing/Temporary/test_invert.mhd -p true 1 -q -f)
	# TODO: check output?
endif()

if (HigherOrderAccurateGradient_LOADED)
	TARGET_COMPILE_DEFINITIONS(CommonImageFilters PRIVATE ITKHigherOrderGradient)
endif()

if (${ITK_USE_GPU} STREQUAL "OFF" OR FLATPAK_BUILD)
	TARGET_COMPILE_DEFINITIONS(CommonImageFilters PRIVATE ITKNOGPU)
endif()