TEST_SRC_DIR=$1
TEST_BIN_DIR=$2
CONFIG_FILE=$3
CTEST_MODE=Nightly
if [ -n "$4" ];
then
	CTEST_MODE=$4
	echo "Using $CTEST_MODE CTest mode!"
fi
COMPILER=GCC
if [ -n "$5" ];
then
	COMPILER=$5
	echo "Compiler=$COMPILER!"
fi
buildtool=make
if [ -n "$6" ];
then
	buildtool=$6
fi
TEST_DIR=$TEST_SRC_DIR/test
if [ -n "$7" ];
then
	TEST_DIR=$7
fi
MODULE_DIRS=$TEST_SRC_DIR/modules
if [ -n "$8" ];
then
	MODULE_DIRS=$8
fi

curdatetime=$(date +"%Y%m%d_%H%M%S")
echo "Automated build at $curdatetime, mode $CTEST_MODE"

TEST_CONFIG_DIR=$(mktemp --tmpdir=/tmp -d ctestconfigs.XXXXXXXXXX)
set NIGHTLY_BUILD_DIR=/mnt/sf_Testdata/Releases/nightly

cd $TEST_SRC_DIR
GIT_BRANCH=$(git symbolic-ref --short HEAD)
# git pull origin master # done automatically by CTest!

# in case it exists, remove output directory:
rm -rf $TEST_BIN_DIR
# create output directory:
mkdir -p $TEST_BIN_DIR
cd $TEST_BIN_DIR

# Set up basic build environment
cmake -C $CONFIG_FILE $TEST_SRC_DIR 2>&1

# Create test configurations:
mkdir -p $TEST_CONFIG_DIR
echo $MODULE_DIRS
python $TEST_DIR/CreateTestConfigurations.py $TEST_SRC_DIR $GIT_BRANCH $TEST_CONFIG_DIR $MODULE_DIRS $COMPILER

# Run with all flags enabled:
cmake -C $TEST_CONFIG_DIR/all_flags.cmake $TEST_SRC_DIR 2>&1
$buildtool clean
ctest -D $CTEST_MODE

if [ "$?" -eq "0" ]
then
	# Create nightly build:
	# Create a tag so that the nightly build gets a proper version name
	curdate = $(date.exe +"%Y.%m.%d")
	cd $TEST_SRC_DIR
	git tag $curdate-nightly
	cd $TEST_BIN_DIR
	# Re-run CMake and build to apply tag:
	cmake $TEST_SRC_DIR
	$buildtool
	# Pack release:
	cpack
	# Move into release directory:
	mv *.sh $NIGHTLY_BUILD_DIR
	mv *.sha512 $NIGHTLY_BUILD_DIR
	# ToDo: Upload (github? git.3dct.at?)
fi

# Run with no flags enabled:
cmake -C $TEST_CONFIG_DIR/no_flags.cmake $TEST_SRC_DIR 2>&1
$buildtool clean
ctest -D Experimental
rm ${TEST_BIN_DIR}/Testing/Temporary/*.mhd ${TEST_BIN_DIR}/Testing/Temporary/*.raw

# iterate over module tests, run build&tests for each:
for modulefile in $TEST_CONFIG_DIR/Module_*.cmake
do
	echo
	echo "--------------------------------------------------------------------------------"
	echo `basename $modulefile .cmake`
	cmake -C ${TEST_CONFIG_DIR}/no_flags.cmake $TEST_SRC_DIR 2>&1
	cmake -C $modulefile $TEST_SRC_DIR 2>&1
	$buildtool clean
	ctest -D Experimental
	rm ${TEST_BIN_DIR}/Testing/Temporary/*.mhd ${TEST_BIN_DIR}/Testing/Temporary/*.raw
done

# CLEANUP:
# remove test configurations:
rm -r $TEST_CONFIG_DIR
rm -r $TEST_BIN_DIR
