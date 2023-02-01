set(MODULE_NAME Remote)

if (Qt${QT_VERSION_MAJOR}HttpServer_FOUND)
	find_program(NPM_PROGRAM npm)
	message(STATUS "NPM: ${NPM_PROGRAM}")
	if(NOT NPM_PROGRAM)
		message(SEND_ERROR "NPM not found; required for building client part of ${MODULE_NAME} module! Please install node.js and/or add it to the path")
	endif()
	target_compile_definitions(${MODULE_NAME} PRIVATE QT_HTTPSERVER)

	set(HTTP_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_NAME}/web")
	set(HTTP_BIN_DIR "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}.dir/web")
	set(HTTP_INSTALL_SRC_DIR "${HTTP_BIN_DIR}/dist_angular")
	set(HTTP_DST_SUBDIR "RemoteClient")
	message(STATUS "source: ${HTTP_SRC_DIR}; bin: ${HTTP_BIN_DIR}; install: ${HTTP_INSTALL_SRC_DIR}")
	set(MULTI_RUNNER "")
	if (WIN32)
		set(MULTI_RUNNER "call")	# NPM is a .bat file under windows, therefore needs to be called with CALL
	endif()
	# first, copy required files to binary folder...
	add_custom_target(filecopying ALL COMMAND ${CMAKE_COMMAND} -E copy_directory ${HTTP_SRC_DIR} ${HTTP_BIN_DIR})
	# ... then call npm run installbuild
	add_custom_command(TARGET ${MODULE_NAME} POST_BUILD
		BYPRODUCTS ${HTTP_INSTALL_SRC_DIR}/main.js
		DEPENDS filecopying
		# use npm/webpack to create the "main.js" file from the index.js / libraries:
		COMMAND ${MULTI_RUNNER} ${NPM_PROGRAM} run installbuild
		WORKING_DIRECTORY ${HTTP_INSTALL_SRC_DIR}
		# copy the client files to the respective configuration's RemoteClient subfolder:
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${HTTP_INSTALL_SRC_DIR}/ $<IF:$<CONFIG:Debug>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG},$<IF:$<CONFIG:Release>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE},$<IF:$<CONFIG:RelWithDebInfo>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO},${CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL}>>>/${HTTP_DST_SUBDIR})
	install(DIRECTORY "${HTTP_INSTALL_SRC_DIR}/" DESTINATION ${HTTP_DST_SUBDIR})
else()
	message(WARNING "Qt HttpServer is not available! You can still run the Remote Render Server, but you will have to serve the 'RemoteClient' folder separately, e.g. via webpack!")
endif()


if (CUDAToolkit_FOUND)
	target_compile_definitions(${MODULE_NAME} PRIVATE CUDA_AVAILABLE)
endif()