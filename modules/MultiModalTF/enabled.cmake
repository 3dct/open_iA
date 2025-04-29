if (MSVC)
	# Apply file grouping based on regular expressions for Visual Studio IDE.
	source_group("TripleHistogramTF" REGULAR_EXPRESSION "tf_3mod/.*[.](h|cpp)$")
endif()
