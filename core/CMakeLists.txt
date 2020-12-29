#FILE( GLOB_RECURSE open_iA_core_test_srcs "test/*.cpp")

# Compiler Flags
IF (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	# see Ulrich Drepper - How to write Shared Libraries
	#   - fPIC vs. fpic
	#   - why fvisibility=hidden
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpic -fvisibility=hidden")
	SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpic -fvisibility=hidden")
ENDIF()

ADD_SUBDIRECTORY(base)
ADD_SUBDIRECTORY(charts)
ADD_SUBDIRECTORY(src)

# TODO: make configurable!
ADD_SUBDIRECTORY(objectvis)

IF (openiA_TESTING_ENABLED)

	# CMAKE_CURRENT_BINARY_DIR/base include required for open_iA_Core_export.h

	# StringHelperTest
	ADD_EXECUTABLE(StringHelperTest test/iAStringHelperTest.cpp base/iAStringHelper.cpp)
	TARGET_INCLUDE_DIRECTORIES(StringHelperTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/base ${CMAKE_CURRENT_BINARY_DIR}/base)
	TARGET_LINK_LIBRARIES(StringHelperTest PRIVATE ${QT_LIBRARIES})
	target_compile_definitions(StringHelperTest PRIVATE NO_DLL_LINKAGE)
	ADD_TEST(NAME StringHelperTest COMMAND StringHelperTest)
	IF (MSVC)
		STRING(REGEX REPLACE "/" "\\\\" QT_WIN_DLL_DIR ${QT_LIB_DIR})
		SET_TESTS_PROPERTIES(StringHelperTest PROPERTIES ENVIRONMENT "PATH=${QT_WIN_DLL_DIR};$ENV{PATH}")
		SET_TARGET_PROPERTIES(StringHelperTest PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${QT_WIN_DLL_DIR};$ENV{PATH}")
	ENDIF()

	# Vec3Test
	ADD_EXECUTABLE(Vec3Test test/iAVec3Test.cpp)
	TARGET_INCLUDE_DIRECTORIES(Vec3Test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/base ${CMAKE_CURRENT_SOURCE_DIR}/src)
	target_compile_definitions(Vec3Test PRIVATE NO_DLL_LINKAGE)
	ADD_TEST(NAME Vec3Test COMMAND Vec3Test)
	
	ADD_EXECUTABLE(MathUtilTest test/iAMathUtilityTest.cpp base/iAMathUtility.cpp)
	TARGET_INCLUDE_DIRECTORIES(MathUtilTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/base ${CMAKE_CURRENT_BINARY_DIR}/base)
	TARGET_LINK_LIBRARIES(MathUtilTest PRIVATE ${VTK_LIBRARIES})
	target_compile_definitions(MathUtilTest PRIVATE NO_DLL_LINKAGE)
	ADD_TEST(NAME MathUtilTest COMMAND MathUtilTest)

	ADD_EXECUTABLE(FunctionalBoxplotTest test/iAFunctionalBoxplotTest.cpp)
	TARGET_INCLUDE_DIRECTORIES(FunctionalBoxplotTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/base ${CMAKE_CURRENT_SOURCE_DIR}/src)
	target_compile_definitions(FunctionalBoxplotTest PRIVATE NO_DLL_LINKAGE)
	ADD_TEST(NAME FunctionalBoxplotTest COMMAND FunctionalBoxplotTest)

	IF (openiA_USE_IDE_FOLDERS)
		SET_PROPERTY(TARGET StringHelperTest PROPERTY FOLDER "Tests")
		SET_PROPERTY(TARGET Vec3Test PROPERTY FOLDER "Tests")
		SET_PROPERTY(TARGET MathUtilTest PROPERTY FOLDER "Tests")
		SET_PROPERTY(TARGET FunctionalBoxplotTest PROPERTY FOLDER "Tests")
	ENDIF()
ENDIF()

ADD_SUBDIRECTORY(gui)