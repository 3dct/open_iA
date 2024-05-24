target_link_libraries(${libname} PUBLIC iA::guibase)
target_link_libraries(${libname} PRIVATE
	iA::renderer iA::slicer iA::objectvis
)
if (openiA_CHART_OPENGL)
	TARGET_COMPILE_DEFINITIONS(${libname} PRIVATE CHART_OPENGL)
endif()
