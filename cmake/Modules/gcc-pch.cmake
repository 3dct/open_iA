# - Try to find precompiled headers support for GCC 3.4 and 4.x
# Once done this will define:
#
# Variable:
#   PCHSupport_FOUND

IF(CMAKE_COMPILER_IS_GNUCXX)
    EXEC_PROGRAM(
        ${CMAKE_CXX_COMPILER} 
        ARGS                    --version 
        OUTPUT_VARIABLE _compiler_output)
    STRING(REGEX REPLACE ".* ([0-9]\\.[0-9]\\.[0-9]) .*" "\\1" 
           gcc_compiler_version ${_compiler_output})
    #MESSAGE("GCC Version: ${gcc_compiler_version}")
    IF(gcc_compiler_version MATCHES "4\\.[0-9]\\.[0-9]")
        SET(PCHSupport_FOUND TRUE)
    ELSE(gcc_compiler_version MATCHES "4\\.[0-9]\\.[0-9]")
        IF(gcc_compiler_version MATCHES "3\\.4\\.[0-9]")
            SET(PCHSupport_FOUND TRUE)
        ENDIF(gcc_compiler_version MATCHES "3\\.4\\.[0-9]")
    ENDIF(gcc_compiler_version MATCHES "4\\.[0-9]\\.[0-9]")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

MACRO(ADD_PRECOMPILED_HEADER targetName source)

    GET_FILENAME_COMPONENT(filename ${source} NAME)
    SET(outdir "${source}.gch")
    MAKE_DIRECTORY(${outdir})
    SET(output "${outdir}/${CMAKE_BUILD_TYPE}.c++")
    STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" CMAKE_CXX_FLAGS_BUILDTYPENAME)
    SET(compiler_ARGS ${${CMAKE_CXX_FLAGS_BUILDTYPENAME}})

# generic cxx flags:
    FOREACH(item ${CMAKE_CXX_FLAGS})
	    LIST (APPEND compiler_ARGS "${item}")
    ENDFOREACH()
    MESSAGE(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

    MESSAGE(STATUS "${CMAKE_CXX_FLAGS_BUILDTYPENAME}: ${${CMAKE_CXX_FLAGS_BUILDTYPENAME}}")

    STRING(TOUPPER "COMPILE_DEFINITIONS_${CMAKE_BUILD_TYPE}" compiledefs_buildtype_name)

# global (unused):
    #GET_PROPERTY(global_compiledefs GLOBAL PROPERTY COMPILE_DEFINITIONS)
    #MESSAGE(STATUS "global COMPILE_DEFINITIONS: ${global_compiledefs}")
    #GET_PROPERTY(global_defs GLOBAL PROPERTY DEFINITIONS)
    #MESSAGE(STATUS "global DEFINITIONS: ${global_defs}")
    #GET_PROPERTY(global_compileopts GLOBAL PROPERTY COMPILE_OPTIONS)
    #MESSAGE(STATUS "global COMPILE_OPTIONS: ${global_compileopts}")

# target-specific definitions (unused):
    #get_target_property(target_compileopts ${targetName} COMPILE_OPTIONS)
    #message(STATUS "target COMPILE_DEFINITIONS: ${target_compileopts}")
    #get_target_property(target_compiledefs ${targetName} COMPILE_DEFINITIONS)
    #message(STATUS "target COMPILE_DEFINITIONS: ${target_compiledefs}")
    #get_target_property(target_defs ${targetName} DEFINITIONS)
    #message(STATUS "target DEFINITIONS: ${target_defs}")
    #get_target_property(target_buildtype_defs ${targetName} ${compiledefs_buildtype_name})
    #message(STATUS "target build type COMPILE_DEFINITIONS: ${target_buildtype_defs}")

# directory-specific definitions:
    GET_DIRECTORY_PROPERTY(directory_includes INCLUDE_DIRECTORIES)
    FOREACH(item ${directory_includes})
		LIST(APPEND compiler_ARGS " -I${item}")
    ENDFOREACH(item)
    #message(STATUS "directory includes: ${directory_includes}")
    GET_DIRECTORY_PROPERTY(directory_defs DEFINITIONS)
    LIST(APPEND compiler_ARGS ${directory_defs})
    message(STATUS "directory DEFINITIONS: ${directory_defs}")

    #GET_DIRECTORY_PROPERTY(directory_compileopts COMPILE_OPTIONS)
    #message(STATUS "directory COMPILE_OPTIONS: ${directory_compileopts}")

    GET_DIRECTORY_PROPERTY(directory_compiledefs COMPILE_DEFINITIONS)
    if (directory_compiledefs)
        foreach(item ${directory_compiledefs})
            LIST(APPEND compiler_ARGS " -D${item}")
        endforeach()
    endif()
    message(STATUS "directory COMPILE DEFINITIONS: ${directory_compiledefs}")

    #GET_DIRECTORY_PROPERTY(directory_buildtype_compiledefs ${compiledefs_buildtype_name})
    #message(STATUS "directory/build type COMPILE DEFINITIONS: ${directory_buildtype_compiledefs}")

    # qt definitions (not sure where these would already be defined?)
    set (qt_definitions "QT_CORE_LIB;QT_GUI_LIB;QT_NETWORK_LIB;QT_OPENGL_LIB;QT_PRINTSUPPORT_LIB;QT_WIDGETS_LIB;QT_XML_LIB")
    foreach(item ${qt_definitions})
        LIST(APPEND compiler_ARGS " -D${item}")
    endforeach()

    if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        LIST(APPEND compiler_ARGS " -DQT_NO_DEBUG")
    else ()
        LIST(APPEND compiler_ARGS " -DQT_DEBUG")
    endif()

    # these flags discovered through trial and error
    LIST (APPEND compiler_ARGS "-c") # not sure why this is necessary (otherwise complains about missing main - in a precompiled header, what?)
    LIST (APPEND compiler_ARGS "-fPIC")	# not sure where this comes from. maybe standard for cmake?
    # specify "this is a c++ header"
    LIST (APPEND compiler_ARGS " -x c++-header")
    #LIST (APPEND compiler_ARGS "c++-header")
    # output
    LIST (APPEND compiler_ARGS "-o")
    LIST (APPEND compiler_ARGS "${output}")
    # the input
    LIST (APPEND compiler_ARGS "${source}")

    message(STATUS "final compiler ARGS: ${compiler_ARGS}")

    SEPARATE_ARGUMENTS(compiler_ARGS)
    
    ADD_CUSTOM_COMMAND(
        OUTPUT ${output}
        COMMAND ${CMAKE_CXX_COMPILER}
        	${compiler_ARGS}
        DEPENDS ${source}
	VERBATIM)
    ADD_CUSTOM_TARGET(${targetName}_gch DEPENDS ${output})
    ADD_DEPENDENCIES(${targetName} ${targetName}_gch)
    SET_TARGET_PROPERTIES(${targetName} PROPERTIES
        COMPILE_FLAGS "-Winvalid-pch"
    )
        
ENDMACRO(ADD_PRECOMPILED_HEADER)

