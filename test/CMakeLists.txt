# CMAKE_CURRENT_BINARY_DIR/base include required for open_iA_Core_export.h

# StringHelperTest
add_executable(StringHelperTest iAStringHelperTest.cpp ../libs/base/iAStringHelper.cpp)      # for iAbase_export.h
target_include_directories(StringHelperTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libs/base ${CMAKE_CURRENT_BINARY_DIR}/../libs)
target_link_libraries(StringHelperTest PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_compile_definitions(StringHelperTest PRIVATE NO_DLL_LINKAGE)
add_test(NAME StringHelperTest COMMAND StringHelperTest)
if (MSVC)
	string(REGEX REPLACE "/" "\\\\" QT_WIN_DLL_DIR ${QT_LIB_DIR})
	set_tests_properties(StringHelperTest PROPERTIES ENVIRONMENT "PATH=${QT_WIN_DLL_DIR}\\;$ENV{PATH}")
	set_target_properties(StringHelperTest PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${QT_WIN_DLL_DIR};$ENV{PATH}")
endif()
if (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	target_compile_options(StringHelperTest PRIVATE -fPIC)
endif()

# Vec3Test
add_executable(Vec3Test iAVec3Test.cpp)
target_include_directories(Vec3Test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libs/base)
target_compile_definitions(Vec3Test PRIVATE NO_DLL_LINKAGE)
target_link_libraries(Vec3Test PRIVATE Qt${QT_VERSION_MAJOR}::Core)
add_test(NAME Vec3Test COMMAND Vec3Test)

# MathUtilTest
add_executable(MathUtilTest iAMathUtilityTest.cpp ../libs/base/iAMathUtility.cpp)        # for iAbase_export.h
target_include_directories(MathUtilTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libs/base ${CMAKE_CURRENT_BINARY_DIR}/../libs)
target_link_libraries(MathUtilTest PRIVATE Qt${QT_VERSION_MAJOR}::Core)
if (openiA_DEPENDENCY_INFO)
	message(STATUS "MathUtilTest dependencies")
endif()
ADD_VTK_LIBRARIES(MathUtilTest PRIVATE CommonCore)    # for vtkMath
target_compile_definitions(MathUtilTest PRIVATE NO_DLL_LINKAGE)
add_test(NAME MathUtilTest COMMAND MathUtilTest)
if (CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	target_compile_options(MathUtilTest PRIVATE -fPIC)
	target_compile_definitions(MathUtilTest PRIVATE QT_NO_VERSION_TAGGING) # see https://stackoverflow.com/a/46522599
endif()
if (MSVC)
	set_tests_properties(MathUtilTest PROPERTIES ENVIRONMENT "PATH=${TestEnvPath}")
	set_target_properties(MathUtilTest PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${WinDLLPaths};$ENV{PATH}")
endif()

# FunctionalBoxplotTest
add_executable(FunctionalBoxplotTest iAFunctionalBoxplotTest.cpp)
target_include_directories(FunctionalBoxplotTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../libs/base ${CMAKE_CURRENT_SOURCE_DIR}/../Toolkit/FunctionalBoxplot)
target_compile_definitions(FunctionalBoxplotTest PRIVATE NO_DLL_LINKAGE)
add_test(NAME FunctionalBoxplotTest COMMAND FunctionalBoxplotTest)

# MSVC
if (openiA_USE_IDE_FOLDERS)
	set_property(TARGET StringHelperTest PROPERTY FOLDER "Tests")
	set_property(TARGET Vec3Test PROPERTY FOLDER "Tests")
	set_property(TARGET MathUtilTest PROPERTY FOLDER "Tests")
	set_property(TARGET FunctionalBoxplotTest PROPERTY FOLDER "Tests")
endif()
