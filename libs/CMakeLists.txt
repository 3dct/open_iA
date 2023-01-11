set(LIB_TYPE SHARED)
include(Modules)

file(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/*")
if (MSVC)
	# Apply file grouping based on regular expressions for Visual Studio (applies to all projects in current folder)
	source_group("UI Files" REGULAR_EXPRESSION "[.](ui|qrc)$")
	source_group("Generated" REGULAR_EXPRESSION "ui_")
endif()

foreach(libdir ${children})
	# todo: add check on whether any module requires lib (for objectvis only ATM?)
	if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${libdir})

		if (openiA_DEPENDENCY_INFO)
			message(STATUS "Library ${libdir}:")
		endif()
		file( GLOB_RECURSE lib_SRCS "${libdir}/*.cpp" "${libdir}/*.h" )
		#FILE( GLOB_RECURSE lib_UI "${libdir}/*.ui" ) # not needed anymore apparently
		file( GLOB_RECURSE lib_QRC "${libdir}/*.qrc" )

		set(libname "iA${libdir}")
		# Generate core dll file / shared object:
		add_library(${libname} ${LIB_TYPE} ${lib_SRCS} ${lib_QRC})
		add_library(iA::${libdir} ALIAS ${libname})	# alias
		
		set(VTK_REQUIRED_LIBS_PUBLIC)
		set(VTK_REQUIRED_LIBS_PRIVATE)
		#set(ITK_REQUIRED_LIBS_PUBLIC)
		#set(ITK_REQUIRED_LIBS_PRIVATE)
		# link libraries, compile definitions and included directories:
		include(${CMAKE_CURRENT_SOURCE_DIR}/${libdir}/lib.cmake)

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
		vtk_module_autoinit(TARGETS ${libname} MODULES ${VTK_LIBRARIES})

		if (openiA_PRECOMPILE)
			if (CMAKE_MAJOR_VERSION GREATER 4 OR
			(CMAKE_MAJOR_VERSION EQUAL 3 AND CMAKE_MINOR_VERSION GREATER 15))
				include(${libdir}/precompile.cmake)
				target_precompile_headers(${libname} PRIVATE ${OPENIA_PRECOMPILE_HEADERS})
			endif()
		endif()

		TARGET_INCLUDE_DIRECTORIES(${libname} PUBLIC
			${CMAKE_CURRENT_SOURCE_DIR}/${libdir}
			${CMAKE_CURRENT_BINARY_DIR} # for export header
		)
		if (openiA_USE_IDE_FOLDERS)
			SET_PROPERTY(TARGET ${libname} PROPERTY FOLDER "Libraries")
		endif()
		
		if (APPLE)
			include_directories( /System/Library/Frameworks )
			find_library(COCOA_LIBRARY Cocoa)
			find_library(GLUT_LIBRARY GLUT )
			find_library(OpenGL_LIBRARY OpenGL )
			mark_as_advanced(COCOA_LIBRARY
							 GLUT_LIBRARY
							 OpenGL_LIBRARY)
			set(EXTRA_LIBS ${COCOA_LIBRARY} ${GLUT_LIBRARY} ${OpenGL_LIBRARY})
			target_link_libraries(${libname} PRIVATE ${EXTRA_LIBS})
			#MESSAGE(STATUS "Extra Core: ${EXTRA_LIBS}")
		endif()
		
		# Installation
		if (WIN32)
			install(TARGETS ${libname} RUNTIME DESTINATION .)
		elseif (FLATPAK_BUILD)
			install(TARGETS ${libname} LIBRARY DESTINATION lib)
		else ()
			install(TARGETS ${libname} LIBRARY DESTINATION .)
		endif()

		# Compiler Flags
		if (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
			# see Ulrich Drepper - How to write Shared Libraries
			#   (https://software.intel.com/sites/default/files/m/a/1/e/dsohowto.pdf)
			#   (- fPIC vs. fpic) -> fPIC added automatically by CMake)
			#   - why fvisibility=hidden
			target_compile_options(${libname} PRIVATE -fvisibility=hidden)
		endif()
	endif()
endforeach()
