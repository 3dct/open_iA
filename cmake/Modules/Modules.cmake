# Generate full path to the module directory
macro (GET_MODULE_FULL_PATH module_name_in module_full_path_out )
	set(${module_full_path_out} ${CMAKE_CURRENT_SOURCE_DIR}/${module_name_in} )
	if (NOT EXISTS ${${module_full_path_out}})
		# message(WARNING "Module ${module_name_in} not found (searched in ${${module_full_path_out}})!")
		set(${module_full_path_out} "NOTFOUND")
	endif()
endmacro()

# Generate name of module interface class
macro (GET_MODULE_INTERFACE_NAME module_name_in module_interface_name_out )
	set(${module_interface_name_out} "iA${module_name}ModuleInterface" )
endmacro()

# Load modules and fill in the corresponding lists
macro (LOAD_MODULES curdir names_out descriptions_out )
	file(GLOB children RELATIVE ${curdir} ${curdir}/*)
	foreach (child ${children})
		if (IS_DIRECTORY ${curdir}/${child})
			set(MODULE_DESCRIPTION "")
			if (EXISTS ${curdir}/${child}/description.cmake)
				include(${curdir}/${child}/description.cmake )
			endif()
			list(APPEND names ${child})
			if (MODULE_DESCRIPTION STREQUAL "")
				message(WARNING "Description of module ${child} is empty or missing! Please create a description.cmake in the module folder with content like this: set(MODULE_DESCRIPTION \"my module description\")")
				set(MODULE_DESCRIPTION "No description")
			endif()
			list(APPEND descriptions ${MODULE_DESCRIPTION})
		endif()
	endforeach()
	set(${names_out} ${names})
	set(${descriptions_out} ${descriptions})
endmacro()

# Module stores its dependencies in Dependencies.txt file. Check if all the dependencies are enabled
function (MODULE_CHECK_DEPENDENCIES option_name module_full_path )
	set(dependencies_full_path ${module_full_path}/Dependencies.cmake)
	if (EXISTS ${dependencies_full_path} )
		set(ADDITIONAL_MODULE_LIBRARIES)
		set(ADDITIONAL_MODULE_LIBRARIES_DEBUG)
		set(ADDITIONAL_MODULE_LIBRARIES_RELEASE)
		set(ADDITIONAL_MODULE_INCLUDE_DIRS)
		include(${dependencies_full_path} )
		# Modules - new style (only link-dependency instead of compiling all files in)
		foreach (d ${DEPENDENCIES_MODULES} )
			set(d_option  ${option_prefix}${d})
			if (NOT ${d_option} )
				message(WARNING "${option_name} depends on ${d_option}, but ${d_option} is disabled; disabling ${option_name} as well!")
				set(${option_name} "OFF" CACHE BOOL "" FORCE)
				return()
			endif()
		endforeach()
		set(ADDITIONAL_MODULE_MODULES ${DEPENDENCIES_MODULES} PARENT_SCOPE)
		# Cmake defines
		foreach (d ${DEPENDENCIES_CMAKE} )
			if (NOT ${d} )
				message(WARNING "${option_name} requires ${d} to be TRUE; disabling the option!")
				set(${option_name} "OFF" CACHE BOOL "" FORCE)
				return()
			endif()
		endforeach()
		# Libraries
		foreach (l ${DEPENDENCIES_LIBRARIES} )
			list(FIND ADDITIONAL_MODULE_LIBRARIES ${l} HasLib )
			if (HasLib EQUAL -1)
				list(APPEND ADDITIONAL_MODULE_LIBRARIES ${l} )
			else()
				message(STATUS "Duplicate library ${l} in ${option_name}!")
			endif()
		endforeach()
		set(ADDITIONAL_MODULE_LIBRARIES ${ADDITIONAL_MODULE_LIBRARIES} PARENT_SCOPE)
		foreach (l ${DEPENDENCIES_LIBRARIES_DEBUG} )
			list(FIND ADDITIONAL_MODULE_LIBRARIES_DEBUG ${l} HasLib )
			if (HasLib EQUAL -1)
				list(APPEND ADDITIONAL_MODULE_LIBRARIES_DEBUG ${l} )
			else()
				message(STATUS "Duplicate debug library ${l} in ${option_name}!")
			endif()
		endforeach()
		set(ADDITIONAL_MODULE_LIBRARIES_DEBUG ${ADDITIONAL_MODULE_LIBRARIES_DEBUG} PARENT_SCOPE)
		foreach (l ${DEPENDENCIES_LIBRARIES_RELEASE} )
			list(FIND ADDITIONAL_MODULE_LIBRARIES_RELEASE ${l} HasLib )
			if (HasLib EQUAL -1)
				list(APPEND ADDITIONAL_MODULE_LIBRARIES_RELEASE ${l} )
			else()
				message(STATUS "Duplicate release library ${l} in ${option_name}!")
			endif()
		endforeach()
		set(ADDITIONAL_MODULE_LIBRARIES_RELEASE ${ADDITIONAL_MODULE_LIBRARIES_RELEASE} PARENT_SCOPE)
		foreach (i ${DEPENDENCIES_INCLUDE_DIRS} )
			list(FIND ADDITIONAL_MODULE_INCLUDE_DIRS ${i} HasInclude )
			if (HasInclude EQUAL -1)
				if (NOT EXISTS ${i})
					message(WARNING "Include directory ${i}, required by module ${option_name}, does not exist!")
					set(${option_name} "OFF" CACHE BOOL "" FORCE)
					return()
				else()
					list(APPEND ADDITIONAL_MODULE_INCLUDE_DIRS ${i} )
				endif()
				# Add in any case; maybe it just doesn't exist _yet_ (but will be created later in configure/make stage)
			endif()
		endforeach()
		# Toolkit directories
		foreach (td ${DEPENDENCIES_IA_TOOLKIT_DIRS} )
			if (NOT EXISTS ${Toolkit_DIR}/${td})
				message(WARNING "Toolkit directory ${td}, required for module ${option_name}, not found (searched in ${Toolkit_DIR}/${td})!")
			else()
				list(APPEND ADDITIONAL_MODULE_INCLUDE_DIRS ${Toolkit_DIR}/${td})
			endif()
		endforeach()
		set(ADDITIONAL_MODULE_INCLUDE_DIRS ${ADDITIONAL_MODULE_INCLUDE_DIRS} PARENT_SCOPE)
		set(ADDITIONAL_COMPILE_DEFINITIONS ${DEPENDENCIES_COMPILE_DEFINITIONS} PARENT_SCOPE)
		#set(ADDITIONAL_ITK_MODULES ${DEPENDENCIES_ITK_MODULES} PARENT_SCOPE)
		set(ADDITIONAL_VTK_MODULES ${DEPENDENCIES_VTK_MODULES} PARENT_SCOPE)
	else()
		# no dependencies, make sure all are cleared from previous modules:
		set(ADDITIONAL_MODULE_MODULES PARENT_SCOPE)
		set(ADDITIONAL_MODULE_LIBRARIES PARENT_SCOPE)
		set(ADDITIONAL_MODULE_LIBRARIES_DEBUG PARENT_SCOPE)
		set(ADDITIONAL_MODULE_LIBRARIES_RELEASE PARENT_SCOPE)
		set(ADDITIONAL_MODULE_INCLUDE_DIRS PARENT_SCOPE)
		set(ADDITIONAL_COMPILE_DEFINITIONS PARENT_SCOPE)
		#set(ADDITIONAL_ITK_MODULES PARENT_SCOPE)
		set(ADDITIONAL_VTK_MODULES PARENT_SCOPE)
	endif()
endfunction()

macro (MODULE_GENERATE_EXPORT_HEADER module_name)
	set(export_h_file ${CMAKE_CURRENT_BINARY_DIR}/${module_name}_export.h)
	# the content of this file never should need to change
	# so if it exists already, we don't need to write it!
	# if we would, we would force a rebuild of all files including it!
	if (NOT EXISTS ${export_h_file})
		FILE(WRITE ${export_h_file}
"/* This file is autogenerated. DO NOT EDIT! */
#if defined(_MSC_VER) && !defined( NO_DLL_LINKAGE)
	#if defined(${module_name}_EXPORTS)
		#define ${module_name}_API __declspec(dllexport)
	#else
		#define ${module_name}_API __declspec(dllimport)
	#endif
#else // export symbols from dynamic shared objects
	#if  defined(__GNUG__) && !defined( NO_DLL_LINKAGE) && defined(${module_name}_EXPORTS)
		#define ${module_name}_API __attribute__ ((visibility (\"default\")))
	#else
		#define ${module_name}_API
	#endif
#endif")
	endif()
endmacro()

macro (MODULE_GENERATE_INTERFACE_FACTORY ident_module_name, real_module_name)
	set(factory_cpp_file ${CMAKE_CURRENT_BINARY_DIR}/${real_module_name}_factory.cpp)
	GET_MODULE_INTERFACE_NAME( ${real_module_name} module_interface_class_name)
	# the content of this file never should need to change
	# so if it exists already, we don't need to write it!
	# if we would, we would force a rebuild of all files including it!
	if (NOT EXISTS ${factory_cpp_file})
		FILE(WRITE ${factory_cpp_file}
"/* This file is autogenerated. DO NOT EDIT! */
#include \"${ident_module_name}_export.h\"
#include \"${module_interface_class_name}.h\"

#ifndef _MSC_VER
#define __stdcall
#endif\n\n\
// Factory method for module interface, called when this module is loaded
extern \"C\"
{
	// NOTE: In case of a compilation error here, change the iAModuleInterface-derived
	// class name to match what is here, instead of changing this file!
	// THIS FILE IS AUTOGENERATED and WILL BE OVERWRITTEN!
	${ident_module_name}_API iAModuleInterface* __stdcall GetModuleInterface()
	{
		return new ${module_interface_class_name}();
	}
}")
	endif()
endmacro()
