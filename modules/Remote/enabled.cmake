if (Qt${QT_VERSION_MAJOR}HttpServer_FOUND)
	find_program(NPM_PROGRAM npm)
	message(STATUS "NPM: ${NPM_PROGRAM}")
	if(NOT NPM_PROGRAM)
		message(SEND_ERROR "NPM not found; required for building client part of Remote module! Please install node.js and/or add it to the path")
	endif()
	target_compile_definitions(Remote PRIVATE QT_HTTPSERVER)
	set(HTTP_INSTALL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Remote/web/dist")
	set(HTTP_DST_SUBDIR "RemoteClient")
	# workaround: under windows, running the single steps directly causes the custom commands afterwards not to be called anymore
	# no exact idea why yet, because 'call' should only be required for .bat files?
	# watch https://discourse.cmake.org/t/add-custom-command-with-multiple-commands-on-windows/6604 for updates
	set(MULTI_RUNNER "")
	if (WIN32)
		set(MULTI_RUNNER "call")
	endif()
	add_custom_command(TARGET Remote POST_BUILD
		BYPRODUCTS ${HTTP_INSTALL_SRC_DIR}/main.js
		# use npm/webpack to create the "main.js" file from the index.js / libraries:
		COMMAND ${MULTI_RUNNER} ${NPM_PROGRAM} run installbuild
		WORKING_DIRECTORY ${HTTP_INSTALL_SRC_DIR}
		# copy the client files to the respective configuration's RemoteClient subfolder:
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${HTTP_INSTALL_SRC_DIR}/ $<IF:$<CONFIG:Debug>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG},$<IF:$<CONFIG:Release>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE},$<IF:$<CONFIG:RelWithDebInfo>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO},${CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL}>>>/${HTTP_DST_SUBDIR})
	install(DIRECTORY "${HTTP_INSTALL_SRC_DIR}/" DESTINATION ${HTTP_DST_SUBDIR})
else()
	message(WARNING "Qt HttpServer is not available! You can still run the Remote Render Server, but you will have to serve the 'RemoteClient' folder separately, e.g. via webpack!")
endif()
