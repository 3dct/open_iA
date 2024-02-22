include(Modules)
set(option_prefix "Module_")

# List all the modules' subdirectories
LOAD_MODULES( ${CMAKE_CURRENT_SOURCE_DIR}
	MODULE_NAMES
	MODULE_DESCRIPTIONS
)

# Iteratively process modules
list(LENGTH MODULE_NAMES _len )
math(EXPR len "${_len} - 1" )

option (openiA_BUILD_ALL_MODULES "Whether to enable all modules in build. Note: To be able to disable a module, this option must be switched off. If it is enabled, always all modules will be automatically enabled! Default: disabled." OFF)
# create options and enable if all modules should be enabled
foreach (indx RANGE ${len} )
	list(GET MODULE_NAMES ${indx} module_name)
	list(GET MODULE_DESCRIPTIONS ${indx} module_description)
	set(module_option ${option_prefix}${module_name})
	option(${module_option} ${module_description} OFF)
	if (openiA_BUILD_ALL_MODULES)
		set(${module_option} "ON" CACHE BOOL "" FORCE)
	endif()
endforeach()

option (openiA_DEPENDENCY_INFO "Whether detailed dependency information should be shown for each module.\nEnable to see if dependencies for your module are properly recognized." OFF)

if (MSVC)
	set(openiA_MODULE_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)")
	option (openiA_MODULE_GUI_DEPENDENCY "Whether the modules should depend on the GUI application (enabling it makes it easier to develop, as then it becomes possible to set the module as active project, and still start the GUI application for debugging purposes directly; but enabling it also makes the build slightly slower)" ON)
	# Apply file grouping based on regular expressions for Visual Studio (applies to all projects in current folder)
	source_group("UI Files" REGULAR_EXPRESSION "[.](ui|qrc)$")
	source_group("Generated" REGULAR_EXPRESSION "ui_")
else()
	set(openiA_MODULE_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()

set(LIB_TYPE SHARED)

get_filename_component(OpeniASrcDir "../" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
get_filename_component(OpeniABinDir "../" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")


foreach (indx RANGE ${len} )
	list(GET MODULE_NAMES ${indx} module_name)
	# Read and print module description
	GET_MODULE_FULL_PATH ( ${module_name} module_full_path )

	set(module_option ${option_prefix}${module_name})
	if (NOT ${module_option} )
		continue()
	endif()

	# Collect source files from the module directories, except potential *Test.cpp/cxx files
	file(GLOB_RECURSE module_srcs "${module_full_path}/*.cpp" "${module_full_path}/*.cxx" )
	file(GLOB_RECURSE module_test_srcs "${module_full_path}/*Test.cpp" "${module_full_path}/*Test.cxx" )
	if (NOT "${module_test_srcs}" STREQUAL "" )
		list(REMOVE_ITEM module_srcs ${module_test_srcs} )
	endif()
	file(GLOB_RECURSE module_hs "${module_full_path}/*.h" )
	file(GLOB_RECURSE module_hpp "${module_full_path}/*.hpp" )
	list(APPEND module_hs ${module_hpp})
	file(GLOB_RECURSE module_qrc "${module_full_path}/*.qrc" )
	list(APPEND module_srcs ${module_qrc})

	# If there are dependencies do the check
	MODULE_CHECK_DEPENDENCIES( ${module_option} ${module_full_path} )
	# Module can be disabled in the course of the dependency check - if so, we need to skip the rest:
	if (NOT ${module_option} )
		continue()
	endif()

	if (openiA_DEPENDENCY_INFO)
		message(STATUS "Module ${module_name}:")
	endif()
	list(APPEND ENABLED_MODULE_NAMES ${module_name})

	# Generate module interface factory
	MODULE_GENERATE_INTERFACE_FACTORY ( ${module_name} )
	list(APPEND module_srcs "${CMAKE_CURRENT_BINARY_DIR}/${module_name}_factory.cpp")

	add_library(${module_name} ${LIB_TYPE} ${module_srcs} ${module_hs} )
	if(MSVC)
		set_target_properties(${module_name} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
	endif()
	GENERATE_EXPORT_HEADER ( ${module_name} EXPORT_MACRO_NAME ${module_name}_API )
	vtk_module_autoinit(TARGETS ${module_name} MODULES ${VTK_LIBRARIES})
	target_include_directories(${module_name} PUBLIC ${module_full_path} )      # public so depending modules only need to link to the target
	target_include_directories(${module_name} PRIVATE ${OpeniABinDir}/modules ) # _export files

	if (openiA_PRECOMPILE)
		if (CMAKE_VERSION VERSION_GREATER "3.15.99")
			if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${module_name}/precompile.cmake")
				include(${module_name}/precompile.cmake)
				target_precompile_headers(${module_name} PRIVATE ${${module_name}_PRECOMPILE_INCLUDES} )
			endif()
		endif()
	endif()

	# Module dependencies: new style - include directory and link dll:
	foreach (d ${ADDITIONAL_MODULE_MODULES} )
		if (openiA_DEPENDENCY_INFO)
			message(STATUS "    modules: ${ADDITIONAL_MODULE_MODULES}")
		endif()
		GET_MODULE_FULL_PATH ( ${d} depend_full_path )
		if (${depend_full_path} STREQUAL "NOTFOUND")
			message(STATUS "Module ${d}, required for module ${module_name}, not found in the filesystem!")
		else()
			# probably only works if modules are dependent on modules which come before them in the alphabet!
			# TODO: real solution working in all cases would need to sort modules in dependency order!
			target_link_libraries(${module_name} PRIVATE ${d})
			target_include_directories(${module_name} PRIVATE ${depend_full_path})
		endif()
	endforeach()

	# required libraries (either from libs folder, or external, such as astra, eigen, OpenCL, ...)
	if (ADDITIONAL_VTK_MODULES)
		ADD_VTK_LIBRARIES(${module_name} "PUBLIC" "${ADDITIONAL_VTK_MODULES}")
	endif()
	if (ADDITIONAL_MODULE_LIBRARIES)
		if (openiA_DEPENDENCY_INFO)
			message(STATUS "    libs: ${ADDITIONAL_MODULE_LIBRARIES}")
		endif()
		target_link_libraries(${module_name} PRIVATE ${ADDITIONAL_MODULE_LIBRARIES})
	endif()
	if (ADDITIONAL_MODULE_LIBRARIES_DEBUG)
		if (openiA_DEPENDENCY_INFO)
			message(STATUS "    libs-dbg: ${ADDITIONAL_MODULE_LIBRARIES_DEBUG}")
		endif()
		target_link_libraries(${module_name} PRIVATE debug ${ADDITIONAL_MODULE_LIBRARIES_DEBUG})
	endif()
	if (ADDITIONAL_MODULE_LIBRARIES_RELEASE)
		if (openiA_DEPENDENCY_INFO)
			message(STATUS "    libs-rel: ${ADDITIONAL_MODULE_LIBRARIES_RELEASE}")
		endif()
		target_link_libraries(${module_name} PRIVATE optimized ${ADDITIONAL_MODULE_LIBRARIES_RELEASE})
	endif()
	if (ADDITIONAL_MODULE_INCLUDE_DIRS)
		if (openiA_DEPENDENCY_INFO)
			message(STATUS "    include: ${ADDITIONAL_MODULE_INCLUDE_DIRS}")
		endif()
		target_include_directories(${module_name} PRIVATE ${ADDITIONAL_MODULE_INCLUDE_DIRS})
	endif()
	if (ADDITIONAL_COMPILE_DEFINITIONS)
		if (openiA_DEPENDENCY_INFO)
			message(STATUS "    compile-defs: ${ADDITIONAL_COMPILE_DEFINITIONS}")
		endif()
		target_compile_definitions(${module_name} PRIVATE ${ADDITIONAL_COMPILE_DEFINITIONS})
	endif()

	if (openiA_USE_IDE_FOLDERS)
		set_property(TARGET ${module_name} PROPERTY FOLDER "Modules")
	endif()

	if (MSVC)	# .dll's on windows are considered RUNTIME (and MSVC is multi-config)
		set_target_properties(${module_name} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/plugins"
			RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}/plugins"
			RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO}/plugins"
			RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL}/plugins"
		)
	elseif (CMAKE_CONFIGURATION_TYPES)	# on other systems, shared libraries are considered LIBRARY
		set_target_properties(${module_name} PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG}/plugins"
			LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE}/plugins"
			LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO}/plugins"
			LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL}/plugins"
		)
	else()
		set_target_properties(${module_name} PROPERTIES
			LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/plugins")
	endif()

	if (MSVC)
		install(TARGETS ${module_name} RUNTIME DESTINATION plugins)    # install dll in plugins folder

		TARGET_COMPILE_OPTIONS(${module_name} PRIVATE "/bigobj")
		# set up debugging command / environment dll paths
		set_target_properties(${module_name} PROPERTIES VS_DEBUGGER_COMMAND "${CMAKE_BINARY_DIR}\\$(Platform)\\$(Configuration)\\${GUI_EXECUTABLE_FILE}")
		set_target_properties(${module_name} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${WinDLLPaths};%PATH%\nSCIFIO_PATH=${SCIFIO_PATH}")

		if (openiA_MODULE_GUI_DEPENDENCY)
			add_dependencies(${module_name} ${GUI_EXECUTABLE_TARGET})       # dependency to GUI executable for triggering rebuild when starting module
		endif()
	else()
		if (FLATPAK_BUILD)
			install(TARGETS ${module_name} LIBRARY DESTINATION bin/plugins)
		else()
			install(TARGETS ${module_name} LIBRARY DESTINATION plugins)
		endif()
	endif()

	if (FLATPAK_BUILD)
		LIST (APPEND BUNDLE_LIBS "\${CMAKE_INSTALL_PREFIX}/bin/plugins/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}")
	else()
		LIST (APPEND BUNDLE_LIBS "\${CMAKE_INSTALL_PREFIX}/plugins/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}")
	endif()

	if (EXISTS "${module_full_path}/enabled.cmake")
		message(STATUS "    Running ${module_full_path}/enabled.cmake...")
		include("${module_full_path}/enabled.cmake")
	endif()
endforeach()
set(BUNDLE_LIBS "${BUNDLE_LIBS}" PARENT_SCOPE)

if (openiA_BUILD_ALL_MODULES)
	message(STATUS "Enabled all modules (openiA_BUILD_ALL_MODULES is set)")
elseif (NOT "${ENABLED_MODULE_NAMES}" STREQUAL "")
	message(STATUS "Enabled modules:" )
	foreach (m ${ENABLED_MODULE_NAMES})
		message(STATUS "    ${m}")
	endforeach()
endif()

# propagate BUILD_INFO changes to parent scope (easier to do here than in single modules; single modules
# would have to set _both_ current scope and parent scope, otherwise current scope var wouldn't change)
set(BUILD_INFO ${BUILD_INFO} PARENT_SCOPE)
