set (BUILD_INFO "${BUILD_INFO}    \"OpenVR SDK: ${OPENVR_VERSION_MAJOR}.${OPENVR_VERSION_MINOR}.${OPENVR_VERSION_BUILD}\\n\"\n" PARENT_SCOPE)

SET (SKYBOX_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VR/images/")
FOREACH(cfg ${CMAKE_CONFIGURATION_TYPES})
	STRING (TOUPPER "${cfg}" CFG)
	SET (SKYBOX_BIN_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG}}/VR-skybox")
	IF (NOT EXISTS "${SKYBOX_BIN_DIR}")
		FILE(COPY "${SKYBOX_SRC_DIR}" DESTINATION "${SKYBOX_BIN_DIR}")
	ENDIF()
ENDFOREACH()

INSTALL(DIRECTORY "${SKYBOX_SRC_DIR}" DESTINATION VR-skybox)