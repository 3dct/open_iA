# Try to find ASTRA Toolbox
#
# The following variables are set:
#    - ASTRA_TOOLBOX_FOUND           true if ASTRA Toolbox found, false otherwise
#    - ASTRA_TOOLBOX_INCLUDE_DIRS    the include directory
#    - ASTRA_TOOLBOX_LIBRARIES       the libraries to link to

find_package( PackageHandleStandardArgs )

set(ASTRA_TOOLBOX_DIR
	"${ASTRA_TOOLBOX_DIR}"
	CACHE
	PATH
	"Path to search for the ASTRA Toolbox"
)
SET (ASTRA_TOOLBOX_LIBRARIES_DEBUG "ASTRA_TOOLBOX_LIBRARIES-NOTFOUND")
SET (ASTRA_TOOLBOX_LIBRARIES_RELEASE "ASTRA_TOOLBOX_LIBRARIES-NOTFOUND")
SET (ASTRA_TOOLBOX_INCLUDE_DIRS "ASTRA_TOOLBOX_INCLUDE_DIRS-NOTFOUND")

IF (MSVC)
	find_library(ASTRA_TOOLBOX_LIBRARIES_RELEASE AstraCuda64
		PATHS
			${ASTRA_TOOLBOX_DIR}/bin/x64/Release_CUDA
			${ASTRA_TOOLBOX_DIR}/mex
			${ASTRA_TOOLBOX_DIR}/lib
	)
	find_library(ASTRA_TOOLBOX_LIBRARIES_DEBUG AstraCuda64D
		PATHS
			${ASTRA_TOOLBOX_DIR}/bin/x64/Debug_CUDA
			${ASTRA_TOOLBOX_DIR}/lib
	)
ELSE()
	find_library(ASTRA_TOOLBOX_LIBRARIES_RELEASE astra
		PATHS
			${ASTRA_TOOLBOX_DIR}/build/linux/.libs
			${ASTRA_TOOLBOX_DIR}/lib

	)
ENDIF()

find_path(ASTRA_INCLUDE_DIR astra/Algorithm.h
	PATHS
		${ASTRA_TOOLBOX_DIR}/include
)

find_path(BOOST_INCLUDE_DIR boost/shared_ptr.hpp
	PATHS
		${ASTRA_TOOLBOX_DIR}/include
		${ASTRA_TOOLBOX_DIR}/lib/include
		/usr/include
)

IF ("${ASTRA_INCLUDE_DIR}" STREQUAL "${BOOST_INCLUDE_DIR}")
	SET (ASTRA_TOOLBOX_INCLUDE_DIRS ${ASTRA_INCLUDE_DIR})
ELSE()
	SET (ASTRA_TOOLBOX_INCLUDE_DIRS ${ASTRA_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})
ENDIF ()

IF (ASTRA_TOOLBOX_LIBRARIES_RELEASE AND ASTRA_TOOLBOX_INCLUDE_DIRS)
	SET (ASTRA_TOOLBOX_FOUND TRUE)
ELSE()
	SET (ASTRA_TOOLBOX_FOUND FALSE)
ENDIF ()

IF (NOT ASTRA_TOOLBOX_LIBRARIES_DEBUG)
	SET (ASTRA_TOOLBOX_LIBRARIES_DEBUG "${ASTRA_TOOLBOX_LIBRARIES_RELEASE}")
ENDIF()

IF (AstraToolbox_FIND_REQUIRED AND NOT ASTRA_TOOLBOX_FOUND)
	MESSAGE(SEND_ERROR "Astra Toolbox not found, but it is specified to be required! Please set ASTRA_TOOLBOX_DIR!")
ENDIF()

if (ASTRA_TOOLBOX_FOUND)
	execute_process(COMMAND
		"${GIT_EXECUTABLE}" describe --tags
		WORKING_DIRECTORY "${ASTRA_TOOLBOX_DIR}"
		RESULT_VARIABLE astra_describe_res
		OUTPUT_VARIABLE astra_describe_out
		ERROR_VARIABLE astra_describe_err
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	if (${astra_describe_res} EQUAL 0)
		STRING(REPLACE "v" "" ASTRA_VERSION ${astra_describe_out})
	else ()
		set(ASTRA_VERSION "unknown")
		string(FIND "${ASTRA_TOOLBOX_DIR}" "-" ASTRA_DASH_POS REVERSE)
		if (${ASTRA_DASH_POS} GREATER -1)
			MATH(EXPR ASTRA_DASH_POS "${ASTRA_DASH_POS} + 1")
			string(SUBSTRING "${ASTRA_TOOLBOX_DIR}" ${ASTRA_DASH_POS} -1 ASTRA_VERSION)
		endif()
	endif()
endif()