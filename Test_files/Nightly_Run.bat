set TEST_SRC_DIR=%1
set TEST_BIN_DIR=%2
set CONFIG_FILE=%3
set VS_PATH=%4
set CTEST_MODE=Nightly
set BUILD_TYPE=Release
set MSBUILD_OPTS=/t:clean /m
if NOT [%5]==[] set CTEST_MODE=%5
CALL :dequote VS_PATH

python --version >NUL 2>NUL
IF %ERRORLEVEL% NEQ 0 GOTO PythonNotFound

:: for security reasons (as we call deltree on it) we don't make that configurable
set TEST_CONFIG_PATH=C:\tmp\ctest_configurations

:: remove binary directory to start from scratch:
rd /s /q %TEST_BIN_DIR%

::TO PAUSE SYSTEM
:: ping -n 10 127.0.0.1 > null
cd %TEST_SRC_DIR%
FOR /F "tokens=1 delims=" %%A in ('git symbolic-ref --short HEAD') do SET GIT_BRANCH=%%A
rem echo %GIT_BRANCH%

:: git pull origin master

:: Set up Visual Studio Environment for cleaning build
:: amd64 is the target architectur (see http://msdn.microsoft.com/en-us/library/x4d2c09s%28v=vs.80%29.aspx)
call "%VS_PATH%\VC\vcvarsall.bat" amd64

@echo on

:: just in case it doesn't exist yet, create output directory:
md %TEST_BIN_DIR%
cd %TEST_BIN_DIR%

:: Set up basic build environment
cmake -C "%CONFIG_FILE%" %TEST_SRC_DIR%

:: Create test configurations:
md %TEST_CONFIG_PATH%
python %TEST_SRC_DIR%\Test_files\CreateTestConfigurations.py %TEST_SRC_DIR% %GIT_BRANCH% %TEST_CONFIG_PATH%

:: Run with all flags enabled:
cmake -C %TEST_CONFIG_PATH%\all_flags.cmake %TEST_SRC_DIR%
MSBuild "%TEST_BIN_DIR%\open_iA.sln" %MSBUILD_OPTS%
del %TEST_Bin_DIR%\core\moc_*
del %TEST_Bin_DIR%\modules\moc_*
ctest -D %CTEST_MODE% -C %BUILD_TYPE%

:: Run with no flags enabled:
cmake -C %TEST_CONFIG_PATH%\no_flags.cmake %TEST_SRC_DIR%
MSBuild "%TEST_BIN_DIR%\open_iA.sln" %MSBUILD_OPTS%
del %TEST_Bin_DIR%\core\moc_*
del %TEST_Bin_DIR%\modules\moc_*
ctest -D Experimental -C %BUILD_TYPE%

:: iterate over module tests, run build&tests for each:
FOR %%m IN (%TEST_CONFIG_PATH%\Module_*) DO @(
	@echo %%~nm
	cmake -C %TEST_CONFIG_PATH%\no_flags.cmake %TEST_SRC_DIR%
	cmake -C %%m %TEST_SRC_DIR%
	MSBuild "%TEST_BIN_DIR%\open_iA.sln" %MSBUILD_OPTS%
	del %TEST_Bin_DIR%\core\moc_*
	del %TEST_Bin_DIR%\modules\moc_*
	ctest -D Experimental -C %BUILD_TYPE%
)

:: CLEANUP:
:: remove test configurations:
rd /s /q %TEST_CONFIG_PATH%
:: reset site name:
cmake -C %TEST_SRC_DIR%\Test_files\site_reset.cmake .

goto end
:: shutdown -s

:DeQuote
for /f "delims=" %%A in ('echo %%%1%%') do set %1=%%~A
Goto :eof

:PythonNotFound
echo Python wasn't found in the PATH! Please install/set PATH appropriately!
goto end

:end
