# Compiler Flags
IF (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	# see Ulrich Drepper - How to write Shared Libraries
	#   - fPIC vs. fpic
	#   - why fvisibility=hidden
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic -fvisibility=hidden")
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -fvisibility=hidden")
ENDIF()


SET (LIB_TYPE SHARED)
INCLUDE(Modules)

FILE(GLOB children RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/*")
FOREACH(libdir ${children})

	# todo: add check on whether any module requires lib (for objectvis only ATM?)
	IF(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${libdir})
		FILE( GLOB_RECURSE lib_SRCS "${libdir}/*.cpp" "${libdir}/*.h" )

		FILE( GLOB_RECURSE lib_UI "${libdir}/*.ui" )
		list(LENGTH lib_UI uiLEN)
		if (uiLEN GREATER 0)
			QT5_WRAP_UI( lib_SRCS ${lib_UI})
		endif()
		
		FILE( GLOB_RECURSE lib_QRC "${libdir}/*.qrc" )
		list(LENGTH lib_QRC qrcLEN)
		if (qrcLEN GREATER 0)
			QT5_ADD_RESOURCES( lib_SRCS ${lib_QRC} )
		endif()

		set (libname "iA${libdir}")
		# Generate core dll file / shared object:
		add_library(${libname} ${LIB_TYPE} ${lib_SRCS} ${lib_QRC})
		
		# link libraries, compile definitions and included directories:
		INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/${libdir}/lib.cmake)

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
			${CMAKE_CURRENT_BINARY_DIR}) # for export header

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
	ENDIF()
ENDFOREACH()

# maybe enough to do this once for the whole project?
IF (MSVC)
	# Apply file grouping based on regular expressions for Visual Studio IDE.
	SOURCE_GROUP("UI Files" REGULAR_EXPRESSION "[.](ui|qrc)$")
ENDIF()

#ADD_SUBDIRECTORY(base)
#ADD_SUBDIRECTORY(charts)
#ADD_SUBDIRECTORY(src)

# TODO: make configurable!
#ADD_SUBDIRECTORY(objectvis)

#ADD_SUBDIRECTORY(gui)