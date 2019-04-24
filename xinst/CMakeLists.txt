
SET (BUNDLE_APP "\${CMAKE_INSTALL_PREFIX}/open_iA${CMAKE_EXECUTABLE_SUFFIX}")

# apparently fixup_bundle only supports specifying a single app; yet _cmd should require exactly the same apps as standard open_iA
#LIST(APPEND BUNDLE_APPS "\${CMAKE_INSTALL_PREFIX}/open_iA_cmd.exe")
MESSAGE(STATUS "Searching for and installing required libs for App=${BUNDLE_APP} and libs=${BUNDLE_LIBS}")
MESSAGE(STATUS "Searched directories: ${BUNDLE_DIRS}")
INSTALL(CODE "
	SET (BUNDLE_LIBS \"${BUNDLE_LIBS}\")
	file(GLOB_RECURSE QTPLUGINS
		\"\${CMAKE_INSTALL_PREFIX}/platforms/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
	LIST (APPEND BUNDLE_LIBS \"\${QTPLUGINS}\")
	include(BundleUtilities)
	fixup_bundle(\"${BUNDLE_APP}\" \"\${BUNDLE_LIBS}\" \"${BUNDLE_DIRS}\")
	")

if (UNIX)
	install(CODE "include(\"${CMAKE_SOURCE_DIR}/cmake/ChangeRPath.cmake\")")
endif()

