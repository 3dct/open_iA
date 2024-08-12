if (openiA_TESTING_ENABLED)
	add_openfile_test(NAME "FeatureScoutNoAutoIDTest" FILENAME "fibers/5-ROI-FeatureScout-LabelVis-NoAutoID.iaproj")
	add_openfile_test(NAME "FeatureScoutWithAutoIDTest" FILENAME "fibers/5-ROI-FeatureScout-CylinderVis-WithAutoID.iaproj")
	add_openfile_test(NAME "FeatureScoutMeanObjects" FILENAME "fibers/5-ROI-FeatureScout-MeanObjects.iaproj")
endif()

