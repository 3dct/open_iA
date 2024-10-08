# This file is for cpack configuration only.
# It should only contain cpack-specific configuration instructions,
# e.g. no install or fixup_bundle code (which would be written to cmake_install.cmake, which we don't want).
# It also must be in the top project folder.
set(CPACK_SOURCE_GENERATOR "ZIP")
if (WIN32)
	set(CPACK_GENERATOR "ZIP")
elseif (UNIX)
	set(CPACK_SOURCE_GENERATOR "TXZ")
	set(CPACK_GENERATOR "STGZ")
elseif (APPLE)
	set(CPACK_GENERATOR "DragNDrop")
endif()
set(CPACK_STRIP_FILES TRUE)
set(CPACK_PACKAGE_NAME "open_iA")
set(CPACK_PACKAGE_VENDOR "University of Applied Sciences Upper Austria, Campus Wels")
set(CPACK_PACKAGE_CONTACT "bernhard.froehler@fh-wels.at")
set(CPACK_PACKAGE_VERSION "${openiA_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A tool for the visual analysis and processing of volumetric datasets, with a focus on industrial computed tomography.")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}-${openiA_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${openiA_VERSION}")
set(CPACK_PACKAGE_CHECKSUM "SHA512")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${openiA_VERSION}-source")

include(CPack)
