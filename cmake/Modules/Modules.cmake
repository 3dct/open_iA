# Generate full path to the module directory
MACRO( GET_MODULE_FULL_PATH module_name_in module_full_path_out )
	SET( ${module_full_path_out} ${CMAKE_CURRENT_SOURCE_DIR}/${module_name_in} )
	IF (NOT EXISTS ${${module_full_path_out}})
		# MESSAGE(WARNING "Module ${module_name_in} not found (searched in ${${module_full_path_out}})!")
		SET( ${module_full_path_out} "NOTFOUND")
	ENDIF()
ENDMACRO()

# Generate name of module interface class
MACRO( GET_MODULE_INTERFACE_NAME module_name_in module_interface_name_out )
	SET( ${module_interface_name_out} "iA${module_name}ModuleInterface" )
ENDMACRO()

# Load modules and fill in the corresponding lists
MACRO( LOAD_MODULES curdir names_out descriptions_out )
	FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
	FOREACH(child ${children})
		IF(IS_DIRECTORY ${curdir}/${child})
			SET(MODULE_DESCRIPTION "")
			IF (EXISTS ${curdir}/${child}/description.cmake)
				INCLUDE( ${curdir}/${child}/description.cmake )
			ENDIF()
			LIST(APPEND names ${child})
			IF (MODULE_DESCRIPTION STREQUAL "")
				MESSAGE(WARNING "Description of module ${child} is empty or missing! Please create a description.cmake in the module folder with content like this: SET(MODULE_DESCRIPTION \"my module description\")")
				SET (MODULE_DESCRIPTION "No description")
			ENDIF()
			LIST(APPEND descriptions ${MODULE_DESCRIPTION})
		ENDIF()
	ENDFOREACH()
	SET(${names_out} ${names})
	SET(${descriptions_out} ${descriptions})
ENDMACRO()

# Module stores its dependencies in Dependencies.txt file. Check if all the dependencies are enabled
FUNCTION( MODULE_CHECK_DEPENDENCIES option_name module_full_path )
	SET( dependencies_full_path ${module_full_path}/Dependencies.cmake)
	IF( EXISTS ${dependencies_full_path} )
		SET (ADDITIONAL_MODULE_LIBRARIES)
		SET (ADDITIONAL_MODULE_LIBRARIES_DEBUG)
		SET (ADDITIONAL_MODULE_LIBRARIES_RELEASE)
		SET (ADDITIONAL_MODULE_INCLUDE_DIRS)
		INCLUDE( ${dependencies_full_path} )
		# Modules - new style (only link-dependency instead of compiling all files in)
		FOREACH( d ${DEPENDENCIES_MODULES} )
			SET( d_option  ${option_prefix}${d})
			IF( NOT ${d_option} )
				MESSAGE(SEND_ERROR "${option_name} depends on ${d_option}")
			ENDIF()
		ENDFOREACH()
		SET (ADDITIONAL_MODULE_MODULES ${DEPENDENCIES_MODULES} PARENT_SCOPE)
		# Cmake defines
		FOREACH( d ${DEPENDENCIES_CMAKE} )
			IF( NOT ${d} )
				MESSAGE(WARNING "${option_name} requires ${d} to be TRUE; disabling the option!")
				set(${option_name} "OFF" CACHE BOOL "" FORCE)
				RETURN ()
			ENDIF()
		ENDFOREACH()
		# Libraries
		FOREACH( l ${DEPENDENCIES_LIBRARIES} )
			list( FIND ADDITIONAL_MODULE_LIBRARIES ${l} HasLib )
			IF(HasLib EQUAL -1)
				LIST( APPEND ADDITIONAL_MODULE_LIBRARIES ${l} )
			ENDIF()
		ENDFOREACH()
		SET (ADDITIONAL_MODULE_LIBRARIES ${ADDITIONAL_MODULE_LIBRARIES} PARENT_SCOPE)
		FOREACH( l ${DEPENDENCIES_LIBRARIES_DEBUG} )
			list( FIND ADDITIONAL_MODULE_LIBRARIES_DEBUG ${l} HasLib )
			IF(HasLib EQUAL -1)
				LIST( APPEND ADDITIONAL_MODULE_LIBRARIES_DEBUG ${l} )
			ENDIF()
		ENDFOREACH()
		SET (ADDITIONAL_MODULE_LIBRARIES_DEBUG ${ADDITIONAL_MODULE_LIBRARIES_DEBUG} PARENT_SCOPE)
		FOREACH( l ${DEPENDENCIES_LIBRARIES_RELEASE} )
			list( FIND ADDITIONAL_MODULE_LIBRARIES_RELEASE ${l} HasLib )
			IF(HasLib EQUAL -1)
				LIST( APPEND ADDITIONAL_MODULE_LIBRARIES_RELEASE ${l} )
			ENDIF()
		ENDFOREACH()
		SET (ADDITIONAL_MODULE_LIBRARIES_RELEASE ${ADDITIONAL_MODULE_LIBRARIES_RELEASE} PARENT_SCOPE)
		FOREACH( i ${DEPENDENCIES_INCLUDE_DIRS} )
			list( FIND ADDITIONAL_MODULE_INCLUDE_DIRS ${i} HasInclude )
			IF (HasInclude EQUAL -1)
				IF (NOT EXISTS ${i})
					MESSAGE(WARNING "Include directory ${i}, required by module ${option_name}, does not exist!")
				ENDIF()
				# Add in any case; maybe it just doesn't exist _yet_ (but will be created later in configure/make stage)
				LIST( APPEND ADDITIONAL_MODULE_INCLUDE_DIRS ${i} )
			ENDIF()
		ENDFOREACH()
		# Toolkit directories
		FOREACH( td ${DEPENDENCIES_IA_TOOLKIT_DIRS} )
			IF (NOT EXISTS ${Toolkit_DIR}/${td})
				MESSAGE(WARNING "Toolkit directory ${td}, required for module ${option_name}, not found (searched in ${Toolkit_DIR}/${td})!")
			ELSE()
				LIST (APPEND ADDITIONAL_MODULE_INCLUDE_DIRS ${Toolkit_DIR}/${td})
			ENDIF()
		ENDFOREACH()
		SET (ADDITIONAL_MODULE_INCLUDE_DIRS ${ADDITIONAL_MODULE_INCLUDE_DIRS} PARENT_SCOPE)
		SET (ADDITIONAL_COMPILE_DEFINITIONS ${DEPENDENCIES_COMPILE_DEFINITIONS} PARENT_SCOPE)
		#SET (ADDITIONAL_ITK_MODULES ${DEPENDENCIES_ITK_MODULES} PARENT_SCOPE)
		SET (ADDITIONAL_VTK_MODULES ${DEPENDENCIES_VTK_MODULES} PARENT_SCOPE)
	ELSE()
		# no dependencies, make sure all are cleared from previous modules:
		SET (ADDITIONAL_MODULE_MODULES PARENT_SCOPE)
		SET (ADDITIONAL_MODULE_LIBRARIES PARENT_SCOPE)
		SET (ADDITIONAL_MODULE_LIBRARIES_DEBUG PARENT_SCOPE)
		SET (ADDITIONAL_MODULE_LIBRARIES_RELEASE PARENT_SCOPE)
		SET (ADDITIONAL_MODULE_INCLUDE_DIRS PARENT_SCOPE)
		SET (ADDITIONAL_COMPILE_DEFINITIONS PARENT_SCOPE)
		#SET (ADDITIONAL_ITK_MODULES PARENT_SCOPE)
		SET (ADDITIONAL_VTK_MODULES PARENT_SCOPE)
	ENDIF()
ENDFUNCTION()

MACRO( MODULE_GENERATE_EXPORT_HEADER module_name)
	SET(export_h_file ${CMAKE_CURRENT_BINARY_DIR}/${module_name}_export.h)
	# the content of this file never should need to change
	# so if it exists already, we don't need to write it!
	# if we would, we would force a rebuild of all files including it!
	IF(NOT EXISTS ${export_h_file})
		FILE(WRITE ${export_h_file}
"/* This file is autogenerated, do not edit*/\n\
#if defined(_MSC_VER) && !defined( NO_DLL_LINKAGE)\n\
	#if defined(${module_name}_EXPORTS)\n\
		#define ${module_name}_API __declspec(dllexport)\n\
	#else\n\
		#define ${module_name}_API __declspec(dllimport)\n\
	#endif\n\
#else // export symbols from dynamic shared objects \n\
	#if  defined(__GNUG__) && !defined( NO_DLL_LINKAGE) && defined(${module_name}_EXPORTS)\n\
		#define ${module_name}_API __attribute__ ((visibility (\"default\")))\n\
	#else \n\
		#define ${module_name}_API\n\
	#endif\n\
#endif")
	ENDIF()
ENDMACRO()

MACRO( MODULE_GENERATE_INTERFACE_FACTORY ident_module_name, real_module_name)
	SET(factory_cpp_file ${CMAKE_CURRENT_BINARY_DIR}/${real_module_name}_factory.cpp)
	GET_MODULE_INTERFACE_NAME( ${real_module_name} module_interface_class_name)
	# the content of this file never should need to change
	# so if it exists already, we don't need to write it!
	# if we would, we would force a rebuild of all files including it!
	IF(NOT EXISTS ${factory_cpp_file})
		FILE(WRITE ${factory_cpp_file}
"/* This file is autogenerated. DO NOT EDIT! */\n\
#include \"${ident_module_name}_export.h\"\n\
#include \"${module_interface_class_name}.h\"\n\n\
#ifndef _MSC_VER\n\
#define __stdcall\n\
#endif\n\n\
// Factory method for module interface, called when this module is loaded\n\
extern \"C\"\n\
{\n\
	// NOTE: In case of a compilation error here, change the iAModuleInterface-derived\n
	// class name to match what is here, instead of this file!\n
	// THIS FILE IS AUTOGENERATED and WILL BE OVERWRITTEN!\n
	${ident_module_name}_API iAModuleInterface* __stdcall GetModuleInterface()\n\
	{\n\
		// NOTE: In case of a compilation error here, change the iAModuleInterface-derived\n
		// class name to match what is here, instead of this file!\n
		// THIS FILE IS AUTOGENERATED and WILL BE OVERWRITTEN!\n
		return new ${module_interface_class_name}();\n\
	}\n\
}\n")
	ENDIF()
ENDMACRO()
