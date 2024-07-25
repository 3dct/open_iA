if (NOT FLATPAK_BUILD)
	# Regexes - initial inspiration from Professional CMake 18th edition
	if (UNIX)
		#include(GNUInstallDirs)
		install(RUNTIME_DEPENDENCY_SET iADependencySet
			PRE_EXCLUDE_REGEXES
				[[\libc\.so\..*]]
				[[libgcc_s\.so\..*]]
				[[libm\.so\..*]]
				[[libstdc\+\+\.so\..*]]
				[[iabase\.dll]]
				[[iacharts\.dll]]
				[[iaguibase\.dll]]
				[[iaio\.dll]]
				[[iaobjectvis\.dll]]
				[[iarenderer\.dll]]
				[[iaslicer\.dll]]
			DIRECTORIES	${BUNDLE_DIRS}
			DESTINATION .
		)
	else()
		install(RUNTIME_DEPENDENCY_SET iADependencySet
			PRE_EXCLUDE_REGEXES
				[[api-ms-win-.*]]
				[[ext-ms-.*]]
				[[hvsifile.*]]
				[[pdmutil.*]]
				[[iabase\.dll]]
				[[iacharts\.dll]]
				[[iaguibase\.dll]]
				[[iaio\.dll]]
				[[iaobjectvis\.dll]]
				[[iarenderer\.dll]]
				[[iaslicer\.dll]]
			POST_EXCLUDE_REGEXES
				[[.*system32.*\.dll]]
			DIRECTORIES	${BUNDLE_DIRS}
			DESTINATION .
		)
	endif()
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
