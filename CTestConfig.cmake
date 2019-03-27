## This file should be placed in the root directory of your project.
## Then modify the CMakeLists.txt file in the root directory of your
## project to incorporate the testing dashboard.

## # The following are required to uses Dart and the Cdash dashboard
##   ENABLE_TESTING()
##   INCLUDE(CTest)
set(CTEST_PROJECT_NAME "openiA")
set(CTEST_NIGHTLY_START_TIME "00:01:00 CEST")

IF (CMAKE_MAJOR_VERSION GREATER 3 OR (CMAKE_MAJOR_VERSION EQUAL 3 AND CMAKE_MINOR_VERSION GREATER 13))
	set(CTEST_SUBMIT_URL "https://git.3dct.at/CDash/submit.php?project=${CTEST_PROJECT_NAME}")
ELSE()
	set(CTEST_DROP_METHOD "https")
	set(CTEST_DROP_SITE "git.3dct.at")
	set(CTEST_DROP_LOCATION "/CDash/submit.php?project=${CTEST_PROJECT_NAME}")
ENDIF()
set(CTEST_DROP_SITE_CDASH TRUE)
