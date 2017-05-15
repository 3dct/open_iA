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
SET (ASTRA_TOOLBOX_LIBRARIES "ASTRA_TOOLBOX_LIBRARIES-NOTFOUND")
SET (ASTRA_TOOLBOX_INCLUDE_DIRS "ASTRA_TOOLBOX_INCLUDE_DIRS-NOTFOUND")
  
find_library(ASTRA_TOOLBOX_LIBRARIES AstraCuda64
    PATHS ${ASTRA_TOOLBOX_DIR}/bin/x64/Release_CUDA
)

find_path(ASTRA_TOOLBOX_INCLUDE_DIRS astra/Algorithm.h
	PATHS ${ASTRA_TOOLBOX_DIR}/include
)

IF (ASTRA_TOOLBOX_LIBRARIES AND ASTRA_TOOLBOX_INCLUDE_DIRS)
	SET (ASTRA_TOOLBOX_FOUND TRUE)
ELSE()
	SET (ASTRA_TOOLBOX_FOUND FALSE)
ENDIF ()

IF (AstraToolbox_FIND_REQUIRED AND NOT ASTRA_TOOLBOX_FOUND)
	MESSAGE(SEND_ERROR "Astra Toolbox not found, but it is specified to be required! Please set ASTRA_TOOLBOX_DIR!")
ENDIF()
