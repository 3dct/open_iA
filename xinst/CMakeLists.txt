if (FLATPAK_BUILD)
	message(STATUS "Skipping fixup_bundle for FLATPAK build...")
else()
	# Bundle creation (while installing / cpack'ing, also include dependency libraries):
	set(BUNDLE_APP "\${CMAKE_INSTALL_PREFIX}/${GUI_EXECUTABLE_FILE}")
	install(CODE "
		set(BUNDLE_LIBS \"${BUNDLE_LIBS}\")
		file(GLOB_RECURSE QTPLUGINS
			\"\${CMAKE_INSTALL_PREFIX}/platforms/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")
		list(APPEND BUNDLE_LIBS \"\${QTPLUGINS}\")
		include(BundleUtilities)
		fixup_bundle(\"${BUNDLE_APP}\" \"\${BUNDLE_LIBS}\" \"${BUNDLE_DIRS}\")
		")
	if (UNIX)
		install(CODE "include(\"${CMAKE_CURRENT_SOURCE_DIR}/../cmake/ChangeRPath.cmake\")")
	endif()
endif()

# install qt configuration file required for dark title bar with dark theme on Windows
if (FLATPAK_BUILD)
	install(FILES qt.conf DESTINATION bin)
else()
	install(FILES qt.conf DESTINATION .)
endif()