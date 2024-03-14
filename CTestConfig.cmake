# Required to use CTest / CDash
# enabled via ENABLE_TESTING() / INCLUDE(CTest) in CMakeLists.txt (in cmake/Modules/Common.cmake in our case)
set(CTEST_PROJECT_NAME "open_iA")
set(CTEST_NIGHTLY_START_TIME "14:00:00 UTC")

set(CTEST_DROP_METHOD "https")
set(CTEST_DROP_SITE "cdash3.3dct.at")
set(CTEST_DROP_LOCATION "/submit.php?project=${CTEST_PROJECT_NAME}")
# required since CMake 3.14:
set(CTEST_SUBMIT_URL "${CTEST_DROP_METHOD}://${CTEST_DROP_SITE}${CTEST_DROP_LOCATION}")
set(CTEST_DROP_SITE_CDASH TRUE)
