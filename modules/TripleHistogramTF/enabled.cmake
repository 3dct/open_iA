IF (MSVC)
	# Apply file grouping based on regular expressions for Visual Studio IDE.
	SOURCE_GROUP("TripleHistogramTF" REGULAR_EXPRESSION "tf_3mod/.*[.](h|cpp)$")
ENDIF()
