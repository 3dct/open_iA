SET( MODULE_DESCRIPTION_OUT
	"Datatype conversion"
	PARENT_SCOPE
)
SET( MODULE_DEFAULT_OPTION_VALUE_OUT OFF  PARENT_SCOPE)

IF (openiA_TESTING_ENABLED AND Module_CastingDataconversion)
	ADD_TEST(NAME CMD_DatatypeConversion1 COMMAND ${TEST_CMD_Binary} -r "Datatype Conversion" -i ${TEST_DATA_DIR}/test2x2x2.mhd -o ${CMAKE_BINARY_DIR}/Testing/Temporary/test_datatypeconv1.mhd -p VTK_FLOAT true true 0 0 false 0 1 -q)
	ADD_TEST(NAME CMD_DatatypeConversion2 COMMAND ${TEST_CMD_Binary} -r "Datatype Conversion" -i ${TEST_DATA_DIR}/test2x2x2.mhd -o ${CMAKE_BINARY_DIR}/Testing/Temporary/test_datatypeconv2.mhd -p VTK_UNSIGNED_CHAR true true 0 0 true 0 0 -q)
ENDIF()