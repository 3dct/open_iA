CMAKE_MINIMUM_REQUIRED ( VERSION 3.0 )

PROJECT ( open_iA_modules LANGUAGES CXX )

INCLUDE(${open_iA_SOURCE_DIR}/cmake/Modules/Modules.cmake)
SET (option_prefix "Module_")

# List all the modules' subdirectories
LOAD_MODULES( ${open_iA_modules_SOURCE_DIR}
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

# workaround for ui_xyz.h files:
INCLUDE_DIRECTORIES( ${open_iA_toolkit_BINARY_DIR}) # ui target directory for ui files in toolkit folder
INCLUDE_DIRECTORIES( ${open_iA_modules_BINARY_DIR} ) # ui target directory for ui files in module folders

INCLUDE_DIRECTORIES( ${open_iA_core_SOURCE_DIR}/src )
INCLUDE_DIRECTORIES( ${open_iA_core_BINARY_DIR} )

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
		LIST(APPEND ENABLED_MODULE_NAMES ${module_name})
		# Collect .cpp and .h sources from the  module directory
		FILE( GLOB module_srcs "${module_full_path}/*.cpp" "${module_full_path}/*.cxx" )
		FILE( GLOB module_test_srcs "${module_full_path}/*Test.cpp" "${module_full_path}/*Test.cxx" )
		FILE( GLOB module_hs "${module_full_path}/*.h" )
		IF ( NOT "${module_test_srcs}" STREQUAL "" )
			# MESSAGE (STATUS "Removing test sources: ${module_test_srcs}")
			LIST( REMOVE_ITEM module_srcs ${module_test_srcs} )
		ENDIF ( NOT "${module_test_srcs}" STREQUAL "" )
		# Collect .ui files from the  module directory and add them
		FILE( GLOB module_uis "${module_full_path}/*.ui" )
		QT5_WRAP_UI( module_srcs ${module_uis})
		# If there are dependencies do the check
		MODULE_CHECK_DEPENDENCIES( ${module_option} ${module_full_path} module_dependencies )
		
		FOREACH( d ${module_dependencies} )
			GET_MODULE_FULL_PATH ( ${d} depend_full_path )
			IF (${depend_full_path} STREQUAL "NOTFOUND")
				MESSAGE(STATUS "Module ${d}, required for module ${module_name}, not found in the filesystem!")
			ELSE()
				FILE( GLOB depend_srcs "${depend_full_path}/*.cpp" "${depend_full_path}/*.cxx" )
				FILE( GLOB depend_test_srcs "${depend_full_path}/*Test.cpp" "${depend_full_path}/*Test.cxx" )
				FILE( GLOB depend_hs "${depend_full_path}/*.h" )
				IF ( NOT "${depend_test_srcs}" STREQUAL "" )
					# MESSAGE (STATUS "Removing test sources: ${depend_test_srcs}")
					LIST( REMOVE_ITEM depend_srcs ${depend_test_srcs} )
				ENDIF ( NOT "${depend_test_srcs}" STREQUAL "" )
				# Collect .ui files from the  module directory and add them
				FILE( GLOB depend_uis "${depend_full_path}/*.ui" )
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
		FOREACH( d ${module_dependencies} )
			GET_MODULE_FULL_PATH ( ${d} depend_full_path )
			TARGET_INCLUDE_DIRECTORIES(${module_name} PRIVATE ${depend_full_path} )
		ENDFOREACH()
		
		IF (NOT "${module_dependencies}" STREQUAL "")
			ADD_DEPENDENCIES(${module_name} ${module_dependencies})
		ENDIF()
		
		# TODO: make depending libraries configurable per module?
		TARGET_LINK_LIBRARIES(${module_name} ${CORE_LIBRARY_NAME})
		TARGET_LINK_LIBRARIES(${module_name} ${QT_LIBRARIES})
		TARGET_LINK_LIBRARIES(${module_name} ${VTK_LIBRARIES})
		TARGET_LINK_LIBRARIES(${module_name} ${ITK_LIBRARIES})
		TARGET_LINK_LIBRARIES(${module_name} ${ADDITIONAL_MODULE_LIBRARIES})
		IF(FFTW_FOUND)
			TARGET_LINK_LIBRARIES(${module_name} ${FFTW_LIBRARIES})
		ENDIF(FFTW_FOUND)
		IF(open_iA_GPU_USING_OPENCL)
			TARGET_LINK_LIBRARIES(${module_name} ${OPENCL_LIBRARIES}) #TODO: proper OpenCL connection
		ENDIF(open_iA_GPU_USING_OPENCL)
		
		IF (MSVC)
			set_target_properties(${module_name} PROPERTIES
				RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/x64/Debug/plugins"
				RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/x64/Release/plugins"
				RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/x64/RelWithDebInfo/plugins"
				RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/x64/MinSizeRel/plugins"
			)
		ELSE(MSVC)
			set_target_properties(${module_name} PROPERTIES
				LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/plugins")
		ENDIF(MSVC)
		
		# Installation
		INSTALL (TARGETS ${module_name} DESTINATION bin/plugins)
	ENDIF( ${module_option} )
ENDFOREACH()

IF (MODULES_BUILD_ALL)
	MESSAGE(STATUS "Enabled all modules (MODULES_BUILD_ALL is set)")
ELSEIF (NOT "${ENABLED_MODULE_NAMES}" STREQUAL "")
	MESSAGE( STATUS "Adding modules: ${ENABLED_MODULE_NAMES}" )
ENDIF ()

# Windows-specific configuration
IF (WIN32)
	ADD_DEFINITIONS(-DCL_COMP)
	IF (MSVC)
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /EHsc")
		# Activate use of multi-processor compilation if we have Microsft Visual C++ compiler.
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

		ADD_DEFINITIONS(/D _CRT_SECURE_NO_WARNINGS)
		ADD_DEFINITIONS(/D _SCL_SECURE_NO_WARNINGS)

		# Apply file grouping based on regular expressions for Visual Studio IDE.
		SOURCE_GROUP("UI Files" REGULAR_EXPRESSION "[.](ui|qrc)$")
	ENDIF (MSVC)
ENDIF (WIN32)
