if (openiA_TESTING_ENABLED)
	# TODO: quite some overlap between here and gui subfolder; unify?
	if (MSVC)
		# use the created .bat which makes sure all required dll's are in the path:
		set(GUITestBinary "${CMAKE_BINARY_DIR}/x64/open_iA_gui_Release.bat")
	elseif(APPLE)
		set(GUITestBinary "${CMAKE_BINARY_DIR}/x64/Release/open_iA.app/Contents/MacOS/open_iA")
	else()
		# make sure the Mesa OpenGL Override version is set
		set(GUITestBinary "${CMAKE_SOURCE_DIR}/test/run_with_gl.sh" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${GUI_EXECUTABLE_TARGET}")
	endif()

	set(TEST_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Testing/Temporary)
	set(TEST_TEMPLATE_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/test/run_test_with_img_output.cmake.in")

	set(TEST_NAME "FeatureScoutNoAutoIDTest")
	set(TEST_OUTPUT_FILE ${TEST_OUTPUT_DIR}/${TEST_NAME}.png)
	set(TEST_COMMAND ${GUITestBinary} ${CMAKE_SOURCE_DIR}/test/data/fibers/5-ROI-FeatureScout-LabelVis-NoAutoID.iaproj --quit 250 --screenshot ${TEST_OUTPUT_FILE} --size 1920x1080)
	set(TEST_SCRIPT_NAME "${CMAKE_BINARY_DIR}/${TEST_NAME}.cmake")
	configure_file(${TEST_TEMPLATE_SCRIPT_FILE} ${TEST_SCRIPT_NAME} @ONLY)
	add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_COMMAND} -P ${TEST_SCRIPT_NAME})

	set(TEST_NAME "FeatureScoutWithAutoIDTest")
	set(TEST_OUTPUT_FILE ${TEST_OUTPUT_DIR}/${TEST_NAME}.png)
	set(TEST_COMMAND ${GUITestBinary} ${CMAKE_SOURCE_DIR}/test/data/fibers/5-ROI-FeatureScout-CylinderVis-WithAutoID.iaproj --quit 250 --screenshot ${TEST_OUTPUT_FILE} --size 1920x1080)
	set(TEST_SCRIPT_NAME "${CMAKE_BINARY_DIR}/${TEST_NAME}.cmake")
	configure_file(${TEST_TEMPLATE_SCRIPT_FILE} ${TEST_SCRIPT_NAME} @ONLY)
	add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_COMMAND} -P ${TEST_SCRIPT_NAME})

	set(TEST_NAME "FeatureScoutMeanObjects")
	set(TEST_OUTPUT_FILE ${TEST_OUTPUT_DIR}/${TEST_NAME}.png)
	set(TEST_COMMAND ${GUITestBinary} ${CMAKE_SOURCE_DIR}/test/data/fibers/5-ROI-FeatureScout-MeanObjects.iaproj --quit 250 --screenshot ${TEST_OUTPUT_FILE} --size 1920x1080)
	set(TEST_SCRIPT_NAME "${CMAKE_BINARY_DIR}/${TEST_NAME}.cmake")
	configure_file(${TEST_TEMPLATE_SCRIPT_FILE} ${TEST_SCRIPT_NAME} @ONLY)
	add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_COMMAND} -P ${TEST_SCRIPT_NAME})
endif()

