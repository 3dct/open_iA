# Required to use CTest / CDash
# enabled via ENABLE_TESTING() / INCLUDE(CTest) in CMakeLists.txt (in cmake/Modules/Common.cmake in our case)
set(CTEST_PROJECT_NAME "openiA")
set(CTEST_NIGHTLY_START_TIME "21:00:00 UTC")

IF (CMAKE_MAJOR_VERSION GREATER 3 OR (CMAKE_MAJOR_VERSION EQUAL 3 AND CMAKE_MINOR_VERSION GREATER 13))
	set(CTEST_SUBMIT_URL "https://cdash.3dct.at/submit.php?project=${CTEST_PROJECT_NAME}")
ELSE()
	set(CTEST_DROP_METHOD "https")
	set(CTEST_DROP_SITE "cdash.3dct.at")
	set(CTEST_DROP_LOCATION "/submit.php?project=${CTEST_PROJECT_NAME}")
ENDIF()
set(CTEST_DROP_SITE_CDASH TRUE)
