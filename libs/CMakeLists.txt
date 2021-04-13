SET (LIB_TYPE SHARED)
INCLUDE(Modules)

FILE(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/*")
FOREACH(libdir ${children})

	# todo: add check on whether any module requires lib (for objectvis only ATM?)
	IF(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${libdir})

		IF (openiA_DEPENDENCY_INFO)
			MESSAGE(STATUS "Library ${libdir}:")
		ENDIF()
		FILE( GLOB_RECURSE lib_SRCS "${libdir}/*.cpp" "${libdir}/*.h" )
		FILE( GLOB_RECURSE lib_UI "${libdir}/*.ui" )
		FILE( GLOB_RECURSE lib_QRC "${libdir}/*.qrc" )

		set (libname "iA${libdir}")
		# Generate core dll file / shared object:
		add_library(${libname} ${LIB_TYPE} ${lib_SRCS} ${lib_QRC})
		
		SET(VTK_REQUIRED_LIBS_PUBLIC)
		SET(VTK_REQUIRED_LIBS_PRIVATE)
		#SET(ITK_REQUIRED_LIBS_PUBLIC)
		#SET(ITK_REQUIRED_LIBS_PRIVATE)
		# link libraries, compile definitions and included directories:
		INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/${libdir}/lib.cmake)

		# simplify / remove duplication ( if/endif / public/private ITK/VTK ) ?
		if (VTK_REQUIRED_LIBS_PUBLIC)
			ADD_VTK_LIBRARIES(${libname} "PUBLIC" "${VTK_REQUIRED_LIBS_PUBLIC}")
		endif()
		if (VTK_REQUIRED_LIBS_PRIVATE)
			ADD_VTK_LIBRARIES(${libname} "PRIVATE" "${VTK_REQUIRED_LIBS_PRIVATE}")
		endif()

		#if (ITK_REQUIRED_LIBS_PUBLIC)
		#	ADD_LEGACY_LIBRARIES(${libname} "" "PUBLIC" "${ITK_REQUIRED_LIBS_PUBLIC}")
		#endif()
		#if (ITK_REQUIRED_LIBS_PRIVATE)
		#	ADD_LEGACY_LIBRARIES(${libname} "" "PRIVATE" "${ITK_REQUIRED_LIBS_PRIVATE}")
		#endif()

		MODULE_GENERATE_EXPORT_HEADER ( ${libname} )

		if (VTK_MAJOR_VERSION GREATER 8)
			vtk_module_autoinit(TARGETS ${libname} MODULES ${VTK_LIBRARIES})
		endif()

		IF (openiA_PRECOMPILE)
			IF (CMAKE_MAJOR_VERSION GREATER 4 OR
			(CMAKE_MAJOR_VERSION EQUAL 3 AND CMAKE_MINOR_VERSION GREATER 15))
				INCLUDE(${libdir}/precompile.cmake)
				target_precompile_headers(${libname} PRIVATE ${OPENIA_PRECOMPILE_HEADERS})
			ENDIF()
		ENDIF()

		TARGET_INCLUDE_DIRECTORIES(${libname} PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}/${libdir}
			${CMAKE_CURRENT_BINARY_DIR} # for export header
		)
		# for generated ui_xxx.h includes:
		IF (CMAKE_CONFIGURATION_TYPES)
			TARGET_INCLUDE_DIRECTORIES(${libname} PUBLIC
				"${CMAKE_CURRENT_BINARY_DIR}/${libname}_autogen/include_$(Configuration)")
		ELSE()
			TARGET_INCLUDE_DIRECTORIES(${libname} PUBLIC
				"${CMAKE_CURRENT_BINARY_DIR}/${libname}_autogen/include")
		ENDIF()

		IF (openiA_USE_IDE_FOLDERS)
			SET_PROPERTY(TARGET ${libname} PROPERTY FOLDER "Libraries")
		ENDIF()
		
		IF(APPLE)
			INCLUDE_DIRECTORIES( /System/Library/Frameworks )
			FIND_LIBRARY(COCOA_LIBRARY Cocoa)
			FIND_LIBRARY(GLUT_LIBRARY GLUT )
			FIND_LIBRARY(OpenGL_LIBRARY OpenGL )
			MARK_AS_ADVANCED(COCOA_LIBRARY
							 GLUT_LIBRARY
							 OpenGL_LIBRARY)
			SET(EXTRA_LIBS ${COCOA_LIBRARY} ${GLUT_LIBRARY} ${OpenGL_LIBRARY})
			TARGET_LINK_LIBRARIES(${libname} PRIVATE ${EXTRA_LIBS})
			#MESSAGE(STATUS "Extra Core: ${EXTRA_LIBS}")
		ENDIF (APPLE)
		
		# Installation
		IF (WIN32)
			INSTALL (TARGETS ${libname} RUNTIME DESTINATION .)
		ELSEIF (FLATPAK_BUILD)
			INSTALL (TARGETS ${libname} LIBRARY DESTINATION lib)
		ELSE ()
			INSTALL (TARGETS ${libname} LIBRARY DESTINATION .)
		ENDIF(WIN32)
		# Compiler Flags
		IF (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
			# see Ulrich Drepper - How to write Shared Libraries
			#   (https://software.intel.com/sites/default/files/m/a/1/e/dsohowto.pdf)
			#   (- fPIC vs. fpic) -> fPIC added automatically by CMake)
			#   - why fvisibility=hidden
			target_compile_options(${libname} PRIVATE -fvisibility=hidden)
		ENDIF()
	ENDIF()
ENDFOREACH()
