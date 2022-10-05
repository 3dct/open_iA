if (Qt${QT_VERSION_MAJOR}HttpServer_FOUND)
	target_compile_definitions(Remote PRIVATE QT_HTTPSERVER)
	set(HTTP_INSTALL_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Remote/web/dist")
	add_custom_command(TARGET Remote PRE_BUILD
		BYPRODUCTS ${HTTP_INSTALL_SRC_DIR}/main.js
		COMMAND npm run installbuild
		WORKING_DIRECTORY ${HTTP_INSTALL_SRC_DIR})
	set(HTTP_DST_SUBDIR "RemoteClient")
	add_custom_command(TARGET Remote POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${HTTP_INSTALL_SRC_DIR}/ $<IF:$<CONFIG:Debug>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG},$<IF:$<CONFIG:Release>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE},$<IF:$<CONFIG:RelWithDebInfo>,${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO},${CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL}>>>/${HTTP_DST_SUBDIR})
	install(DIRECTORY "${HTTP_INSTALL_SRC_DIR}/" DESTINATION ${HTTP_DST_SUBDIR})
else()
	message(WARNING "Qt HttpServer is not available! You can still run the Remote Render Server, but you will have to serve the 'RemoteClient' folder separately, e.g. via webpack!")
endif()
