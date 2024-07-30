# Generate executable:
set(MAIN_SOURCES "${CMAKE_SOURCE_DIR}/gui/main.cpp")

if (WIN32)
	set(MAIN_SOURCES ${MAIN_SOURCES} open_iA.rc)
	add_executable( ${GUI_EXECUTABLE_TARGET} WIN32 ${MAIN_SOURCES})
elseif(APPLE)
	set(ICNS_FILE_NAME open_iA.icns)
	set(MACOSX_BUNDLE_ICON_FILE ${ICNS_FILE_NAME})
	set(macos_app_icon ${CMAKE_CURRENT_SOURCE_DIR}/${ICNS_FILE_NAME})
	set_source_files_properties(${macos_app_icon} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources") # takes care of installing file?
	qt_add_executable( ${GUI_EXECUTABLE_TARGET} MACOSX_BUNDLE ${MAIN_SOURCES} ${macos_app_icon})
	set(MACOSX_BUNDLE_GUI_IDENTIFIER at.zfp.openia)
else()
	add_executable( ${GUI_EXECUTABLE_TARGET} ${MAIN_SOURCES} )
endif()

target_link_libraries(${GUI_EXECUTABLE_TARGET} PRIVATE iA::gui)

target_include_directories(${GUI_EXECUTABLE_TARGET} PRIVATE ${CMAKE_BINARY_DIR}) # for version.h

# Windows-specific configuration
if (MSVC)
	if (NOT EXISTS ${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj.user
		OR NOT EXISTS ${CMAKE_BINARY_DIR}/x64/open_iA_gui_Release.bat
		OR NOT "${WinDLLPaths}" STREQUAL "${CACHED_OPENIA_GUI_ENV_PATH}"
	)
		message(STATUS "GUI LIBRARY paths changed new=${WinDLLPaths}, cached=${CACHED_OPENIA_GUI_ENV_PATH}")
		# force cmake to copy the files:
		file(REMOVE ${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj.user)
		# create .user file FOR ALL_BUILD (for which we cannot use set_property / set_target_properties):
		configure_file(${CMAKE_SOURCE_DIR}/gui/ALL_BUILD.vcxproj.user ${CMAKE_BINARY_DIR}/ @ONLY)
		# make sure visual studio reloads the projects:
		execute_process(COMMAND cmake -E touch ${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj)
		set(CACHED_OPENIA_GUI_ENV_PATH "${WinDLLPaths}" CACHE INTERNAL "" FORCE)
		set(EXE_NAME ${GUI_EXECUTABLE_FILE})
		foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
			string(REPLACE "$(Configuration)" "${CONFIG}" ADDITIONAL_PATH "${WinDLLPaths}")
			file(REMOVE ${CMAKE_BINARY_DIR}/x64/open_iA_gui_${CONFIG}.bat)
			configure_file(${CMAKE_SOURCE_DIR}/cmake/run-tmpl.bat ${CMAKE_BINARY_DIR}/x64/open_iA_gui_${CONFIG}.bat @ONLY)
		endforeach()
	endif()
	set_target_properties(${GUI_EXECUTABLE_TARGET} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${WinDLLPaths};%PATH%\nSCIFIO_PATH=${SCIFIO_PATH}")
	target_sources(${GUI_EXECUTABLE_TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/cmake/WindowsApplicationUseUtf8.manifest)
endif()

if (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	target_compile_options(${GUI_EXECUTABLE_TARGET} PRIVATE -fPIE)
endif()

if (openiA_TESTING_ENABLED)
	if (MSVC)
		# use the created .bat which makes sure all required dll's are in the path:
		set(FileOpenTestBinary "${CMAKE_BINARY_DIR}/x64/open_iA_gui_Release.bat")
	elseif(APPLE)
		set(FileOpenTestBinary "${CMAKE_BINARY_DIR}/x64/Release/open_iA.app/Contents/MacOS/open_iA")
	else()
		# make sure the Mesa OpenGL Override version is set
		set(FileOpenTestBinary "${CMAKE_SOURCE_DIR}/test/run_with_gl.sh" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${GUI_EXECUTABLE_TARGET}")
	endif()

	set(TEST_TEMPLATE_SCRIPT_FILE "${CMAKE_SOURCE_DIR}/test/run_test_with_img_output.cmake.in")
	set(TEST_NAME "FileOpenTest")
	set(TEST_OUTPUT_FILE ${CMAKE_BINARY_DIR}/Testing/Temporary/${TEST_NAME}.png)
	set(TEST_COMMAND ${FileOpenTestBinary} ${CMAKE_SOURCE_DIR}/test/data/test2x2x2.mhd --quit 100 --screenshot ${TEST_OUTPUT_FILE})
	set(TEST_SCRIPT_NAME "${CMAKE_BINARY_DIR}/${TEST_NAME}.cmake")
	configure_file(${TEST_TEMPLATE_SCRIPT_FILE} ${TEST_SCRIPT_NAME} @ONLY)
	add_test(NAME ${TEST_NAME} COMMAND ${CMAKE_COMMAND} -P ${TEST_SCRIPT_NAME})
endif()


# Installation
if (FLATPAK_BUILD)
	install(TARGETS ${GUI_EXECUTABLE_TARGET} RUNTIME DESTINATION bin)
	# TODO: also install .desktop for non-flatpak build (with open_iA exec name)
	install(FILES ${FLATPAK_APPID}.desktop DESTINATION share/applications/)
	install(FILES ${FLATPAK_APPID}.svg DESTINATION share/icons/hicolor/32x32/apps/)
elseif (APPLE)
	install(TARGETS ${GUI_EXECUTABLE_TARGET} BUNDLE DESTINATION .)
else()
	install(TARGETS ${GUI_EXECUTABLE_TARGET} RUNTIME_DEPENDENCY_SET iADependencySet RUNTIME DESTINATION .)
endif()
