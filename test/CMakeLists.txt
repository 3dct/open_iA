# CMAKE_CURRENT_BINARY_DIR/base include required for open_iA_Core_export.h

# StringHelperTest
ADD_EXECUTABLE(StringHelperTest iAStringHelperTest.cpp ../libs/base/iAStringHelper.cpp)
TARGET_INCLUDE_DIRECTORIES(StringHelperTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libs/base ${CMAKE_CURRENT_BINARY_DIR}/../libs)
TARGET_LINK_LIBRARIES(StringHelperTest PRIVATE ${QT_LIBRARIES})
target_compile_definitions(StringHelperTest PRIVATE NO_DLL_LINKAGE)
ADD_TEST(NAME StringHelperTest COMMAND StringHelperTest)
IF (MSVC)
	STRING(REGEX REPLACE "/" "\\\\" QT_WIN_DLL_DIR ${QT_LIB_DIR})
	SET_TESTS_PROPERTIES(StringHelperTest PROPERTIES ENVIRONMENT "PATH=${QT_WIN_DLL_DIR};$ENV{PATH}")
	SET_TARGET_PROPERTIES(StringHelperTest PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${QT_WIN_DLL_DIR};$ENV{PATH}")
ENDIF()

# Vec3Test
ADD_EXECUTABLE(Vec3Test iAVec3Test.cpp)
TARGET_INCLUDE_DIRECTORIES(Vec3Test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libs/base ${CMAKE_CURRENT_SOURCE_DIR}/../libs/core)
target_compile_definitions(Vec3Test PRIVATE NO_DLL_LINKAGE)
ADD_TEST(NAME Vec3Test COMMAND Vec3Test)

ADD_EXECUTABLE(MathUtilTest iAMathUtilityTest.cpp ../libs/base/iAMathUtility.cpp)
TARGET_INCLUDE_DIRECTORIES(MathUtilTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libs/base ${CMAKE_CURRENT_BINARY_DIR}/../libs)
TARGET_LINK_LIBRARIES(MathUtilTest PRIVATE ${VTK_LIBRARIES})
target_compile_definitions(MathUtilTest PRIVATE NO_DLL_LINKAGE)
ADD_TEST(NAME MathUtilTest COMMAND MathUtilTest)

ADD_EXECUTABLE(FunctionalBoxplotTest iAFunctionalBoxplotTest.cpp)
TARGET_INCLUDE_DIRECTORIES(FunctionalBoxplotTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libs/base ${CMAKE_CURRENT_SOURCE_DIR}/../Toolkit/FunctionalBoxplot)
target_compile_definitions(FunctionalBoxplotTest PRIVATE NO_DLL_LINKAGE)
ADD_TEST(NAME FunctionalBoxplotTest COMMAND FunctionalBoxplotTest)

IF (openiA_USE_IDE_FOLDERS)
	SET_PROPERTY(TARGET StringHelperTest PROPERTY FOLDER "Tests")
	SET_PROPERTY(TARGET Vec3Test PROPERTY FOLDER "Tests")
	SET_PROPERTY(TARGET MathUtilTest PROPERTY FOLDER "Tests")
	SET_PROPERTY(TARGET FunctionalBoxplotTest PROPERTY FOLDER "Tests")
ENDIF()
