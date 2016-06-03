# Generate full path to the module directory
MACRO( GET_MODULE_FULL_PATH module_name_in module_full_path_out )
    SET( ${module_full_path_out} ${PROJECT_SOURCE_DIR}/${module_name_in} )
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
MACRO( LOAD_MODULES curdir names_out descriptions_out def_vals_out )
    FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
    FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
        ADD_SUBDIRECTORY( ${curdir}/${child} )
        LIST(APPEND names ${child})
        LIST(APPEND descriptions ${MODULE_DESCRIPTION_OUT})
        LIST(APPEND default_vals ${MODULE_DEFAULT_OPTION_VALUE_OUT})
    ENDIF()
    ENDFOREACH()
    SET(${names_out} ${names})
    SET(${descriptions_out} ${descriptions})
    SET(${def_vals_out} ${default_vals})
ENDMACRO()

# Module stores its dependencies in Dependencies.txt file. Check if all the dependencies are enabled
MACRO( MODULE_CHECK_DEPENDENCIES option_name module_full_path module_dependencies)

    SET( dependencies_full_path ${module_full_path}/Dependencies.cmake)
    IF( EXISTS ${dependencies_full_path} )
        INCLUDE( ${dependencies_full_path} )
        # Modules
        FOREACH( d ${DEPENDENCIES_MODULES} )
            SET( d_option  ${option_prefix}${d})
            IF( NOT ${d_option} )
                MESSAGE(SEND_ERROR "${option_name} depends on ${d_option}")
            ENDIF()
        ENDFOREACH()
		SET (module_dependencies ${DEPENDENCIES_MODULES})
        # Cmake defines
        FOREACH( d ${DEPENDENCIES_CMAKE} )
            IF( NOT ${d} )
                MESSAGE(SEND_ERROR "${option_name} requires ${d} to be ON")
            ENDIF()
        ENDFOREACH()
        # Libraries
        FOREACH( l ${DEPENDENCIES_LIBRARIES} )
            list( FIND ADDITIONAL_MODULE_LIBRARIES ${l} HasLib )
            IF(HasLib EQUAL -1)
                LIST( APPEND ADDITIONAL_MODULE_LIBRARIES ${l} )
            ENDIF()
        ENDFOREACH()
        # Toolkit directories
        FOREACH( td ${DEPENDENCIES_IA_TOOLKIT_DIRS} )
            INCLUDE_DIRECTORIES( ${Toolkit_DIR}/${td} )
			IF (NOT EXISTS ${Toolkit_DIR}/${td})
				MESSAGE(WARNING "Required toolkit directory ${td} for module ${option_name} not found (searched in ${Toolkit_DIR}/${td})!")
			ENDIF()
        ENDFOREACH()
    ENDIF()
ENDMACRO()

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
#else // no export specification needed for tests and other platforms\n\
	#define ${module_name}_API\n\
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
"/* This file is autogenerated, do not edit*/\n\
#include \"pch.h\"\n\
#include \"${ident_module_name}_export.h\"\n\
#include \"${module_interface_class_name}.h\"\n\n\
#ifndef _MSC_VER\n\
#define __stdcall\n\
#endif\n\n\
// Factory method for module interface, called when this module is loaded\n\
extern \"C\"\n\
{\n\
	${ident_module_name}_API iAModuleInterface* __stdcall GetModuleInterface()\n\
	{\n\
		return new ${module_interface_class_name}();\n\
	}\n\
}\n")
	ENDIF()
ENDMACRO()

# Generate a header file including all the module's headers
#MACRO( MODULE_GENERATE_INCLUDE_HEADER module_name module_full_path )
#    FILE( GLOB module_headers "${module_full_path}/*.h" )
#    GET_MODULE_INTERFACE_NAME ( ${module_name} module_interface_class_name )
#    SET(module_interface_class_name "${module_full_path}/${module_interface_class_name}.h")
#    LIST( REMOVE_ITEM module_headers ${module_interface_class_name} )
#    SET( inc_h_name ${inc_h_prefix}${module_name} )
#    SET( inc_h_file ${inc_h_name}.h )
#    SET( inc_h_file_abs ${CMAKE_CURRENT_BINARY_DIR}/${inc_h_file} )
#
#    FILE( WRITE ${inc_h_file_abs}
#        "/* This file is autogenerated, do not edit*/\n#ifndef ${inc_h_name}\n#define ${inc_h_name}\n#ifdef ${module_option}\n\n"
#    )
#
#    FOREACH( h ${module_headers} )
#        GET_FILENAME_COMPONENT( h_name ${h} NAME )
#        FILE( APPEND ${inc_h_file_abs}
#            "#include \"${h_name}\"\n"
#        )
#    ENDFOREACH()
#
#    FILE( APPEND ${inc_h_file_abs}
#    "\n#endif //${module_option}\n#endif //${inc_h_name}\n"
#    )
#ENDMACRO()
