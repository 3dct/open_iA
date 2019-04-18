SET( MODULE_DESCRIPTION_OUT
	"Triple histogram transfer function:\n"
	PARENT_SCOPE
)
SET( MODULE_DEFAULT_OPTION_VALUE_OUT OFF  PARENT_SCOPE)

IF (MSVC)
	# Apply file grouping based on regular expressions for Visual Studio IDE.
	SOURCE_GROUP("TripleHistogramTF" REGULAR_EXPRESSION "TripleHistogramTF/[.](h|cpp)$")
ENDIF()