SET( MODULE_DESCRIPTION_OUT
	"Common image filters:\n- Canny Edge Detection filter\n- Connected component and relabel filters\n- Datatype and range conversion\n- (FFT) Correlation/Convolution\n- Adding noise (additive gaussian, salt and pepper, shot, speckle)\n- Resample image\n- Extract Image\n- Intensity filters: Difference, Subtract, Invert, Mask Image, Intensity Windowing, Normalize Image, Histogram Match\n- Hessian / Eigenanalysis, Particle characterization\n- Morphology filters (Dilation, Erosion, Opening, Closing, ...)\n- Smoothing filters - Edge preserving (Gradient anisotropic diffusion, Curvature anisotropic diffusion, Bilateral image filter) & Blurring (Discrete Gaussian)\n- Transformations (Rotate, Translate, Flip)\n- Distance maps (Danielsson, Signed Maurer)"
PARENT_SCOPE)

SET( MODULE_DEFAULT_OPTION_VALUE_OUT OFF  PARENT_SCOPE)

IF (openiA_TESTING_ENABLED AND Module_CommonImageFilters)
	ADD_TEST(NAME CMD_DatatypeConversion1 COMMAND ${TEST_CMD_Binary} -r "Datatype Conversion" -i ${TEST_DATA_DIR}/test2x2x2.mhd -o ${CMAKE_BINARY_DIR}/Testing/Temporary/test_datatypeconv1.mhd -p "32 bit floating point number (7 digits, float)" true true 0 0 false 0 1 -q -f)

	ADD_TEST(NAME CMD_DatatypeConversion2 COMMAND ${TEST_CMD_Binary} -r "Datatype Conversion" -i ${TEST_DATA_DIR}/test2x2x2.mhd -o ${CMAKE_BINARY_DIR}/Testing/Temporary/test_datatypeconv2.mhd -p "8 bit unsigned integer (0 to 255, unsigned char)" true true 0 0 true 0 0 -q -f)

	ADD_TEST(NAME CMD_Invert COMMAND ${TEST_CMD_Binary} -r "Invert" -i ${TEST_DATA_DIR}/test2x2x2.mhd -o ${CMAKE_BINARY_DIR}/Testing/Temporary/test_invert.mhd -p true 1 -q -f)
	# TODO: check output?
ENDIF()