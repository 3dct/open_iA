set(MODULE_NAME Remote)

set(WebServerConfigured OFF)
if (Qt6HttpServer_FOUND)
	find_program(NPM_PROGRAM npm)
	if (NPM_PROGRAM)
		target_compile_definitions(${MODULE_NAME} PRIVATE QT_HTTPSERVER)

		set(HTTP_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_NAME}/web")
		set(HTTP_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}.dir/web")
		set(HTTP_INSTALL_SRC_DIR "${HTTP_BIN_DIR}/dist_angular")
		set(HTTP_DST_SUBDIR "RemoteClient")
		set(MULTI_RUNNER "")
		if (WIN32)
			set(MULTI_RUNNER "call")	# NPM is a .bat file under windows, therefore needs to be called with CALL
		endif()
		file(GLOB_RECURSE WEB_CLIENT_SRC_FILES
			"${HTTP_SRC_DIR}/*.css"
			"${HTTP_SRC_DIR}/*.html"
			"${HTTP_SRC_DIR}/*.js"
			"${HTTP_SRC_DIR}/*.json"
			"${HTTP_SRC_DIR}/*.scss"
			"${HTTP_SRC_DIR}/*.ts")
		get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
		if (${is_multi_config})
			add_custom_command(OUTPUT ${HTTP_INSTALL_SRC_DIR}/index.html
				# re-run on a change of any of the web client source files:
				DEPENDS ${WEB_CLIENT_SRC_FILES}
				# first, copy required files to binary folder...
				COMMAND ${CMAKE_COMMAND} -E copy_directory ${HTTP_SRC_DIR} ${HTTP_BIN_DIR}
				# ... then use npm/webpack to build everything libraries (this also internally calls angular build somehow):
				COMMAND ${MULTI_RUNNER} ${NPM_PROGRAM} run installbuild
				WORKING_DIRECTORY ${HTTP_INSTALL_SRC_DIR}
				# ... then copy the client files to the respective configuration's RemoteClient subfolder (TODO: test on non-multi-config generators, e.g. ninja):
				COMMAND ${CMAKE_COMMAND} -E copy_directory ${HTTP_INSTALL_SRC_DIR}/ $<IF:$<CONFIG:Debug>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG},$<IF:$<CONFIG:Release>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE},$<IF:$<CONFIG:RelWithDebInfo>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO},${CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL}>>>/${HTTP_DST_SUBDIR})
		else()
			# we only need to vary the last command (no generator expression for non-multi-config generators)
			# but I'm unaware of a way to only switch out generator expression part, as generator expression cannot be stored in a variable
			add_custom_command(OUTPUT ${HTTP_INSTALL_SRC_DIR}/index.html
				# re-run on a change of any of the web client source files:
				DEPENDS ${WEB_CLIENT_SRC_FILES}
				# first, copy required files to binary folder...
				COMMAND ${CMAKE_COMMAND} -E copy_directory ${HTTP_SRC_DIR} ${HTTP_BIN_DIR}
				# ... then use npm/webpack to build everything libraries (this also internally calls angular build somehow):
				COMMAND ${MULTI_RUNNER} ${NPM_PROGRAM} run installbuild
				WORKING_DIRECTORY ${HTTP_INSTALL_SRC_DIR}
				# ... then copy the client files to the respective configuration's RemoteClient subfolder (TODO: test on non-multi-config generators, e.g. ninja):
				COMMAND ${CMAKE_COMMAND} -E copy_directory ${HTTP_INSTALL_SRC_DIR}/ ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${HTTP_DST_SUBDIR})
	
		endif()

		target_sources(${MODULE_NAME} PRIVATE ${HTTP_INSTALL_SRC_DIR}/index.html)
		# ^ weird (feeling like a woraround) kind of way to make the above command actually run,
		# see https://cmake.org/cmake/help/latest/command/add_custom_command.html :
		#   > A target [...] that specifies any output of the custom command as a source file is given a rule to generate the file using the command at build time
		install(DIRECTORY "${HTTP_INSTALL_SRC_DIR}/" DESTINATION ${HTTP_DST_SUBDIR})
		set(WebServerConfigured ON)
	else()
		message(WARNING "NPM not found; required for building client part of ${MODULE_NAME} module! Please install node.js and the npm binary and/or add it to the path")
	endif()
else()
	message(WARNING "Qt Http Server library not available! To install, start Qt Maintenance tool, select 'Add or remove components', and under 'Additional Libraries', select the 'Qt HTTP Server (TP)' option!")
endif()

if ("${WebServerConfigured}" STREQUAL "OFF")
	message(WARNING "The Webserver part of ${MODULE_NAME} could not be set up (see message above for more details). The websocket part will be available, but unless you fix above warnings, you will have to build and serve the HTTP server for serving the client code manually; see README.md in web subfolder!")
endif()


if (CUDAToolkit_FOUND)
	target_compile_definitions(${MODULE_NAME} PRIVATE CUDA_AVAILABLE)
endif()
