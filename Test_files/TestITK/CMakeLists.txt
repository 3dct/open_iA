set(FilterTests
#test no. 1: NCC image filter
itkNormalizedCorrelationImageFilterTest.cxx )

MESSAGE("Make sure network drive NanotomData2 is mapped")

add_executable(NCCTest itkNormalizedCorrelationImageFilterTest.cxx)
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/Test_files/TestITK/x64")
target_link_libraries(NCCTest ${ITK_LIBRARIES})


#add_test(NAME NCCTest -C Release COMMAND $<TARGET_FILE:NCCTest> --compare DATA{${TESTING_DATA_ROOT}/Input/100664_8_PPGF30_2um_EvalSIM_CutoutCenter.mhd}
#		{${TESTING_DATA_ROOT}/Baseline/100664_8_PPGF30_2um_EvalSIM_CutoutCenter.mhd}
#		itkNormalizedCorrelationImageFilterTest 
#		DATA{${TESTING_DATA_ROOT}/Input/100664_8_PPGF30_2um_EvalSIM_CutoutCenter.mhd} 
#		DATA{${TESTING_DATA_ROOT}/Input/template_sphere_d13um.mhd} 
#		${TESTING_DATA_OUTPUT}/NormalizedCorrelationImageFilterTest.mhd)

#setting test parameters (e.g. input file paths) does not work properly. 
#the idea is to add the paths in CMakeLists.txt, build the test in VS, and then build the target RUN_TESTS.
#input paths are currently set in source code. 
set (nccTestParameters "${CMAKE_SOURCE_DIR}/Test_files/TestITK/Input/100664_8_PPGF30_2um_EvalSIM_CutoutCenter.mhd ${CMAKE_SOURCE_DIR}/Test_files/TestITK/Input/template_sphere_d13um.mhd")
add_test(NAME NCCTest COMMAND $<TARGET_FILE:NCCTest> ${nccTestParameters})
