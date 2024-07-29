if (openiA_TESTING_ENABLED)
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
	add_test(NAME FeatureScoutNoAutoIDTest COMMAND ${GUITestBinary} ${CMAKE_SOURCE_DIR}/test/data/fibers/5-ROI-FeatureScout-LabelVis-NoAutoID.iaproj --quit 250 --screenshot ${TEST_OUTPUT_DIR}/5-ROI-FeatureScout-LabelVis-NoAutoID.png)
	add_test(NAME FeatureScoutWithAutoIDTest COMMAND ${GUITestBinary} ${CMAKE_SOURCE_DIR}/test/data/fibers/5-ROI-FeatureScout-CylinderVis-WithAutoID.iaproj --quit 250 --screenshot ${TEST_OUTPUT_DIR}/5-ROI-FeatureScout-CylinderVis-WithAutoID.png)
endif()

