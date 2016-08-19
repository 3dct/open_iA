TEST_SRC_DIR=$1
TEST_BIN_DIR=$2
CONFIG_FILE=$3
CTEST_MODE=Nightly
if [ -n "$4" ];
then
	CTEST_MODE=$4
	echo "Using $CTEST_MODE CTest mode!"
fi
if [ -n "$5" ];
then
	TEST_FILES_DIR=$5
fi
if [ -n "$6" ];
then
	MODULE_DIR=$6
	echo "Using $CTEST_MODE CTest mode!"
fi



TEST_CONFIG_DIR=$(mktemp --tmpdir=/tmp -d ctestconfigs.XXXXXXXXXX)

cd $TEST_SRC_DIR
GIT_BRANCH=$(git symbolic-ref --short HEAD)
# git pull origin master # done automatically by CTest!

# just in case it doesn't exist yet, create output directory:
mkdir -p $TEST_BIN_DIR
cd $TEST_BIN_DIR

# Set up basic build environment
make clean
cmake -C $CONFIG_FILE $TEST_SRC_DIR

# Create test configurations:
mkdir -p $TEST_CONFIG_DIR
python $TEST_FILES_DIR/CreateTestConfigurations.py $TEST_SRC_DIR $GIT_BRANCH $TEST_CONFIG_DIR $MODULE_DIR

# Run with all flags enabled:
cmake -C $TEST_CONFIG_DIR/all_flags.cmake $TEST_SRC_DIR
make clean
ctest -D $CTEST_MODE

# Run with no flags enabled:
cmake -C $TEST_CONFIG_DIR/no_flags.cmake $TEST_SRC_DIR
make clean
ctest -D Experimental

# iterate over module tests, run build&tests for each:
for modulefile in $TEST_CONFIG_DIR/Module_*.cmake
do
	echo
	echo "--------------------------------------------------------------------------------"
	echo `basename $modulefile .cmake`
	cmake -C ${TEST_CONFIG_DIR}/no_flags.cmake $TEST_SRC_DIR
	cmake -C $modulefile $TEST_SRC_DIR
	make clean
	ctest -D Experimental
done

# CLEANUP:
# remove test configurations:
rm -r $TEST_CONFIG_DIR

