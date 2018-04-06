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

option (MODULES_BUILD_ALL "" OFF)
# create options and enable if all modules should be enabled
FOREACH( indx RANGE ${len} )
	LIST( GET MODULE_NAMES ${indx} module_name)
	LIST( GET MODULE_DESCRIPTIONS ${indx} module_description)
	LIST( GET MODULE_DEFAULT_VALS ${indx} module_def_val)
	SET(module_option ${option_prefix}${module_name})
	OPTION( ${module_option} ${module_description} ${module_def_val})
	IF (MODULES_BUILD_ALL)
		set(${module_option} "ON" CACHE BOOL "" FORCE)
	ENDIF()
ENDFOREACH()

SET (LIB_TYPE SHARED)

get_filename_component(OpeniASrcDir "../" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
get_filename_component(OpeniABinDir "../" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")

FOREACH( indx RANGE ${len} )
	LIST( GET MODULE_NAMES ${indx} module_name)
	# Read and print module description
	GET_MODULE_FULL_PATH ( ${module_name} module_full_path )
	# MESSAGE(STATUS "module src patH: ${module_full_path}")
	# Add an option for the module
	SET(module_option ${option_prefix}${module_name})

	# If option is used add the module
	IF( ${module_option} )
		SET(ADDITIONAL_MODULE_LIBRARIES "")
		SET(ADDITIONAL_MODULE_LIBRARIES_DEBUG "")
		SET(ADDITIONAL_MODULE_LIBRARIES_RELEASE "")
		SET(ADDITIONAL_MODULE_INCLUDE_DIRS "")
		LIST(APPEND ENABLED_MODULE_NAMES ${module_name})
		# Collect source files from the module directories, except potential *Test.cpp/cxx files
		FILE( GLOB_RECURSE module_srcs "${module_full_path}/*.cpp" "${module_full_path}/*.cxx" )
		FILE( GLOB_RECURSE module_test_srcs "${module_full_path}/*Test.cpp" "${module_full_path}/*Test.cxx" )
		IF ( NOT "${module_test_srcs}" STREQUAL "" )
			LIST( REMOVE_ITEM module_srcs ${module_test_srcs} )
		ENDIF ()
		FILE( GLOB_RECURSE module_hs "${module_full_path}/*.h" )
		FILE( GLOB_RECURSE module_uis "${module_full_path}/*.ui" )
		QT5_WRAP_UI( module_srcs ${module_uis})
		# If there are dependencies do the check
		MODULE_CHECK_DEPENDENCIES( ${module_option} ${module_full_path} module_dependencies )

		FOREACH( d ${module_dependencies} )
			GET_MODULE_FULL_PATH ( ${d} depend_full_path )
			IF (${depend_full_path} STREQUAL "NOTFOUND")
				MESSAGE(STATUS "Module ${d}, required for module ${module_name}, not found in the filesystem!")
			ELSE()
				FILE( GLOB_RECURSE depend_srcs "${depend_full_path}/*.cpp" "${depend_full_path}/*.cxx" )
				FILE( GLOB_RECURSE depend_test_srcs "${depend_full_path}/*Test.cpp" "${depend_full_path}/*Test.cxx" )
				FILE( GLOB_RECURSE depend_hs "${depend_full_path}/*.h" )
				IF ( NOT "${depend_test_srcs}" STREQUAL "" )
					# MESSAGE (STATUS "Removing test sources: ${depend_test_srcs}")
					LIST( REMOVE_ITEM depend_srcs ${depend_test_srcs} )
				ENDIF ( NOT "${depend_test_srcs}" STREQUAL "" )
				# Collect .ui files from the  module directory and add them
				FILE( GLOB_RECURSE depend_uis "${depend_full_path}/*.ui" )
				QT5_WRAP_UI( depend_srcs ${depend_uis})

				LIST (APPEND module_srcs ${depend_srcs})
				LIST (APPEND module_hs ${depend_hs})
			ENDIF()
		ENDFOREACH()

		# clean module name for use as identifier (if leading number, then add "_" prefix)
		string(REGEX REPLACE "^([0123456789])" "_\\1" ident_module_name ${module_name})
		# Generate module export header
		MODULE_GENERATE_EXPORT_HEADER ( ${ident_module_name} )
		# Generate module interface factory
		MODULE_GENERATE_INTERFACE_FACTORY ( ${ident_module_name}, ${module_name} )
		LIST (APPEND module_srcs "${CMAKE_CURRENT_BINARY_DIR}/${module_name}_factory.cpp")
		# Add preprocessor definition TODO: check if it can be removed!
		ADD_DEFINITIONS( -D${module_option} )

		add_library(${module_name} ${LIB_TYPE} ${module_srcs} ${module_hs} )
		TARGET_INCLUDE_DIRECTORIES(${module_name} PRIVATE ${module_full_path} )
		# workaround for ui_xyz.h files:
		TARGET_INCLUDE_DIRECTORIES(${module_name} PRIVATE ${OpeniABinDir}/Toolkit)  # ui files in toolkit folder
		TARGET_INCLUDE_DIRECTORIES(${module_name} PRIVATE ${OpeniABinDir}/modules ) # ui files in module folders
		FOREACH( d ${module_dependencies} )
			GET_MODULE_FULL_PATH ( ${d} depend_full_path )
			TARGET_INCLUDE_DIRECTORIES(${module_name} PRIVATE ${depend_full_path} )
		ENDFOREACH()

		# set IDE folder:
		SET_PROPERTY(TARGET ${module_name} PROPERTY FOLDER "Modules")

		IF (NOT "${module_dependencies}" STREQUAL "")
			ADD_DEPENDENCIES(${module_name} ${module_dependencies})
		ENDIF()
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
			IF (NOT "${DebugLibraryPaths}" STREQUAL "${CACHED_${module_name}_DBG_ENV_PATH}")
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
			INSTALL (TARGETS ${module_name} LIBRARY DESTINATION plugins)
		ENDIF()
	ENDIF( ${module_option} )
ENDFOREACH()

IF (MODULES_BUILD_ALL)
	MESSAGE(STATUS "Enabled all modules (MODULES_BUILD_ALL is set)")
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
IF (CMAKE_COMPILER_IS_GNUCXX)
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic -fvisibility=hidden")
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -fvisibility=hidden")
ENDIF ()
