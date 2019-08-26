INCLUDE(Modules)
SET (option_prefix "Module_")

# List all the modules' subdirectories
LOAD_MODULES( ${CMAKE_CURRENT_SOURCE_DIR}
	MODULE_NAMES
	MODULE_DESCRIPTIONS
	MODULE_DEFAULT_VALS
)

# Iteratively process modules
LIST( LENGTH MODULE_NAMES _len )
MATH( EXPR len "${_len} - 1" )

option (openiA_BUILD_ALL_MODULES "Whether to enable all modules in build. Note: To be able to disable a module, this option must be switched off. If it is enabled, always all modules will be automatically enabled! Default: disabled." OFF)
# create options and enable if all modules should be enabled
FOREACH( indx RANGE ${len} )
	LIST( GET MODULE_NAMES ${indx} module_name)
	LIST( GET MODULE_DESCRIPTIONS ${indx} module_description)
	LIST( GET MODULE_DEFAULT_VALS ${indx} module_def_val)
	SET(module_option ${option_prefix}${module_name})
	OPTION( ${module_option} ${module_description} ${module_def_val})
	IF (openiA_BUILD_ALL_MODULES)
		set(${module_option} "ON" CACHE BOOL "" FORCE)
	ENDIF()
ENDFOREACH()

IF (MSVC)
	SET (openiA_MODULE_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}/$(Configuration)")
ELSE()
	SET (openiA_MODULE_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}")
ENDIF()

SET (LIB_TYPE SHARED)

get_filename_component(OpeniASrcDir "../" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
get_filename_component(OpeniABinDir "../" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")

FOREACH( indx RANGE ${len} )
	LIST( GET MODULE_NAMES ${indx} module_name)
	# Read and print module description
	GET_MODULE_FULL_PATH ( ${module_name} module_full_path )
	# Add an option for the module
	SET(module_option ${option_prefix}${module_name})

	IF( NOT ${module_option} )
		CONTINUE()
	ENDIF()
	LIST(APPEND ENABLED_MODULE_NAMES ${module_name})
	# Collect source files from the module directories, except potential *Test.cpp/cxx files
	FILE( GLOB_RECURSE module_srcs "${module_full_path}/*.cpp" "${module_full_path}/*.cxx" )
	FILE( GLOB_RECURSE module_test_srcs "${module_full_path}/*Test.cpp" "${module_full_path}/*Test.cxx" )
	IF ( NOT "${module_test_srcs}" STREQUAL "" )
		LIST( REMOVE_ITEM module_srcs ${module_test_srcs} )
	ENDIF ()
	FILE( GLOB_RECURSE module_hs "${module_full_path}/*.h" )
	FILE( GLOB_RECURSE module_hpp "${module_full_path}/*.hpp" )
	LIST (APPEND module_hs ${module_hpp})
	FILE( GLOB_RECURSE module_uis "${module_full_path}/*.ui" )
	QT5_WRAP_UI( module_srcs ${module_uis})
	# If there are dependencies do the check
	MODULE_CHECK_DEPENDENCIES( ${module_option} ${module_full_path} )
	# Module can be disabled in the course of the dependency check - if so, we need to skip the rest:
	IF( NOT ${module_option} )
		CONTINUE()
	ENDIF()

	# clean module name for use as identifier (if leading number, then add "_" prefix)
	string(REGEX REPLACE "^([0123456789])" "_\\1" ident_module_name ${module_name})
	# Generate module export header
	MODULE_GENERATE_EXPORT_HEADER ( ${ident_module_name} )
	# Generate module interface factory
	MODULE_GENERATE_INTERFACE_FACTORY ( ${ident_module_name}, ${module_name} )
	LIST (APPEND module_srcs "${CMAKE_CURRENT_BINARY_DIR}/${module_name}_factory.cpp")

	add_library(${module_name} ${LIB_TYPE} ${module_srcs} ${module_hs} )
	if(MSVC)
		set_target_properties(${module_name} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
	endif()
	TARGET_INCLUDE_DIRECTORIES(${module_name} PRIVATE ${module_full_path} )
	# workaround for ui_xyz.h files:
	TARGET_INCLUDE_DIRECTORIES(${module_name} PRIVATE ${OpeniABinDir}/Toolkit)  # ui files in toolkit folder
	TARGET_INCLUDE_DIRECTORIES(${module_name} PRIVATE ${OpeniABinDir}/modules ) # ui files in module folders
	
	# Module dependencies: new style - include directory and link dll:
	FOREACH( d ${ADDITIONAL_MODULE_MODULES} )
		GET_MODULE_FULL_PATH ( ${d} depend_full_path )
		IF (${depend_full_path} STREQUAL "NOTFOUND")
			MESSAGE(STATUS "Module ${d}, required for module ${module_name}, not found in the filesystem!")
		ELSE()
			# probably only works if modules are dependent on modules which come before them in the alphabet!
			# TODO: real solution working in all cases would need to sort modules in dependency order!
			TARGET_LINK_LIBRARIES(${module_name} PRIVATE ${d})
			TARGET_INCLUDE_DIRECTORIES(${module_name} PRIVATE ${depend_full_path})
		ENDIF()
	ENDFOREACH()

	# also pulls in libraries defined in core interface (qt, vtk, itk):
	TARGET_LINK_LIBRARIES(${module_name} PRIVATE ${CORE_LIBRARY_NAME})

	# additional module (such as astra, eigen, OpenCL, ...)
	IF (ADDITIONAL_MODULE_LIBRARIES)
		TARGET_LINK_LIBRARIES(${module_name} PRIVATE ${ADDITIONAL_MODULE_LIBRARIES})
	ENDIF()
	IF (ADDITIONAL_MODULE_LIBRARIES_DEBUG)
		TARGET_LINK_LIBRARIES(${module_name} PRIVATE debug ${ADDITIONAL_MODULE_LIBRARIES_DEBUG})
	ENDIF()
	IF (ADDITIONAL_MODULE_LIBRARIES_RELEASE)
		TARGET_LINK_LIBRARIES(${module_name} PRIVATE optimized ${ADDITIONAL_MODULE_LIBRARIES_RELEASE})
	ENDIF()
	IF (ADDITIONAL_MODULE_INCLUDE_DIRS)
		TARGET_INCLUDE_DIRECTORIES(${module_name} PRIVATE ${ADDITIONAL_MODULE_INCLUDE_DIRS})
	ENDIF()
	IF (ADDITIONAL_COMPILE_DEFINITIONS)
		TARGET_COMPILE_DEFINITIONS(${module_name} PRIVATE ${ADDITIONAL_COMPILE_DEFINITIONS})
	ENDIF()

	IF (openiA_USE_COTIRE)
		cotire(${module_name})
	ENDIF()

	IF (openiA_USE_IDE_FOLDERS)
		SET_PROPERTY(TARGET ${module_name} PROPERTY FOLDER "Modules")
	ENDIF()

	IF (MSVC)
		# Apply file grouping based on regular expressions for Visual Studio IDE.
		SOURCE_GROUP("UI Files" REGULAR_EXPRESSION "[.](ui|qrc)$")
		set_target_properties(${module_name} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/x64/Debug/plugins"
			RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/x64/Release/plugins"
			RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/x64/RelWithDebInfo/plugins"
			RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/x64/MinSizeRel/plugins"
		)
		INSTALL (TARGETS ${module_name} RUNTIME DESTINATION plugins)

		# Install .vcxproj.user file for module to be able to start open_iA
		IF (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/${module_name}.vcxproj.user OR NOT "${DebugLibraryPaths}" STREQUAL "${CACHED_${module_name}_DBG_ENV_PATH}")
			# force cmake to copy the files:
			FILE(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/${module_name}.vcxproj.user)
			# create .user files:
			CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/module.vcxproj.user ${CMAKE_CURRENT_BINARY_DIR}/${module_name}.vcxproj.user @ONLY)
			# make sure visual studio reloads the projects:
			execute_process(COMMAND cmake -E touch ${CMAKE_CURRENT_BINARY_DIR}/${module_name}.vcxproj)
			SET (CACHED_${module_name}_DBG_ENV_PATH "${DebugLibraryPaths}" CACHE INTERNAL "" FORCE)
		ENDIF ()
	ELSE()
		IF (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
			set_target_properties(${module_name} PROPERTIES
				LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/plugins"
				LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/plugins"
				LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo/plugins"
				LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel/plugins"
		)
		ELSE()
			set_target_properties(${module_name} PROPERTIES
				LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins")
		ENDIF()
		IF (FLATPAK_BUILD)
			INSTALL (TARGETS ${module_name} LIBRARY DESTINATION bin/plugins)
		ELSE()
			INSTALL (TARGETS ${module_name} LIBRARY DESTINATION plugins)
		ENDIF()
	ENDIF()
	
	IF (FLATPAK_BUILD)
		LIST (APPEND BUNDLE_LIBS "\${CMAKE_INSTALL_PREFIX}/bin/plugins/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}")
	ELSE()
		LIST (APPEND BUNDLE_LIBS "\${CMAKE_INSTALL_PREFIX}/plugins/${CMAKE_SHARED_LIBRARY_PREFIX}${module_name}${CMAKE_SHARED_LIBRARY_SUFFIX}")
	ENDIF()
ENDFOREACH()
SET (BUNDLE_LIBS "${BUNDLE_LIBS}" PARENT_SCOPE)

IF (openiA_BUILD_ALL_MODULES)
	MESSAGE(STATUS "Enabled all modules (openiA_BUILD_ALL_MODULES is set)")
ELSEIF (NOT "${ENABLED_MODULE_NAMES}" STREQUAL "")
	MESSAGE( STATUS "Enabled modules:" )
	FOREACH (m ${ENABLED_MODULE_NAMES})
		MESSAGE(STATUS "    ${m}")
	ENDFOREACH()
ENDIF ()

# Compiler flags
IF (MSVC)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
ENDIF (MSVC)
IF (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic -fvisibility=hidden")
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -fvisibility=hidden")
ENDIF ()
