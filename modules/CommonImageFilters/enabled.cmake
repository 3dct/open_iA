if (openiA_TESTING_ENABLED)

	set(TEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Testing/Temporary)

	# TODO: check output?

	add_test(NAME CMD_DatatypeConversion1 COMMAND ${TEST_CMD_Binary} run "Datatype Conversion" -i ${TEST_DATA_DIR}/test2x2x2.mhd -o ${TEST_OUTPUT_DIR}/test_datatypeconv1.mhd -p "32 bit floating point number (7 digits, float)" true true 0 0 false 0 1 -q -f)

	add_test(NAME CMD_DatatypeConversion2 COMMAND ${TEST_CMD_Binary} run "Datatype Conversion" -i ${TEST_DATA_DIR}/test2x2x2.mhd -o ${TEST_OUTPUT_DIR}/test_datatypeconv2.mhd -p "8 bit unsigned integer (0 to 255, unsigned char)" true true 0 0 true 0 0 -q -f)

	add_test(NAME CMD_Invert COMMAND ${TEST_CMD_Binary} run "Invert" -i ${TEST_DATA_DIR}/test2x2x2.mhd -o ${TEST_OUTPUT_DIR}/test_invert.mhd -p true 1 -q -f)

	# Test output parameters:
	add_test(NAME CMD_Invert_Compress COMMAND ${TEST_CMD_Binary} run "Invert" -i ${TEST_DATA_DIR}/test2x2x2.mhd -o ${TEST_OUTPUT_DIR}/test_invert-compress.mhd -k true -p true 1 -q -f)

	# Test multiple input parameters:
	add_test(NAME CMD_AddRaw COMMAND ${TEST_CMD_Binary} run "Add Images" -i ${TEST_DATA_DIR}/test4x4x4.raw ${TEST_DATA_DIR}/test4x4x4.raw -j "4,4,4" "0.01,0.01,0.01" "0,0,0" 0 "32 bit floating point number (7 digits, float)" "Little Endian"  -j "4,4,4" "0.01,0.01,0.01" "0,0,0" 0 "32 bit floating point number (7 digits, float)" "Little Endian" -o ${TEST_OUTPUT_DIR}/test-add.mhd -v 1 -f)

	# TODO: test multiple output parameters, multiple different in/out parameters

endif()

if (HigherOrderAccurateGradient_LOADED)
	TARGET_COMPILE_DEFINITIONS(CommonImageFilters PRIVATE ITKHigherOrderGradient)
endif()

if (${ITK_USE_GPU} STREQUAL "OFF" OR FLATPAK_BUILD)
	TARGET_COMPILE_DEFINITIONS(CommonImageFilters PRIVATE ITKNOGPU)
endif()