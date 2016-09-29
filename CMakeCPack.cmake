# TODO: use VC++ Redistributable installer instead?
#     legally, it shouldn't be a problem though to include the runtime libraries, see e.g.
#     https://stackoverflow.com/questions/35097193/can-i-bundle-the-visual-studio-2015-c-redistributable-dlls-with-my-applicatio
set (CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
include (InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "open_iA")
set(CPACK_PACKAGE_VENDOR "FHW")
include(GetGitRevisionDescription)
git_describe(VERSION --tags)
set(CPACK_PACKAGE_VERSION "${VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A tool for the visual analysis and processing of volumetric datasets, with a focus on industrial computed tomography.")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}-${VERSION}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${VERSION}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${VERSION}-source")
#SET(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/logo.bmp")

set(CPACK_PACKAGE_EXECUTABLES "" "")
set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/gui/open_iA.ico")
set(CPACK_NSIS_MENU_LINKS "./${CPACK_PACKAGE_NAME}.exe" "${CPACK_PACKAGE_NAME} ${VERSION}")
set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME} ${VERSION}")

include(CPack)
