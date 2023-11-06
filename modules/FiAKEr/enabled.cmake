if (openiA_TESTING_ENABLED)
	get_filename_component(CoreSrcDir "../libs/base" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_filename_component(CoreBinDir "../libs" REALPATH BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
	add_executable(MDSTest FiAKEr/iAMultiDimensionalScalingTest.cpp FiAKEr/iAMultidimensionalScaling.cpp)
	target_link_libraries(MDSTest PRIVATE Qt::Core)
	target_include_directories(MDSTest PRIVATE
		${CoreSrcDir}                         # for iAStringHelper, required by MDS
		${CoreBinDir}                         # for iAbase_export.h
		${CMAKE_CURRENT_BINARY_DIR}           # for _export.h
	)
	target_compile_definitions(MDSTest PRIVATE NO_DLL_LINKAGE)
	add_test(NAME MDSTest COMMAND MDSTest)
	if (MSVC)
		cmake_path(NATIVE_PATH QT_LIB_DIR QT_WIN_DLL_DIR)
		set_tests_properties(MDSTest PROPERTIES ENVIRONMENT "PATH=${QT_WIN_DLL_DIR};$ENV{PATH}")
		set_target_properties(MDSTest PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${QT_WIN_DLL_DIR};$ENV{PATH}")
	else()
		target_compile_options(MDSTest PRIVATE -fPIC)
	endif()
	if (openiA_USE_IDE_FOLDERS)
		set_property(TARGET MDSTest PROPERTY FOLDER "Tests")
	endif()
endif()
