@echo off

echo === Running test-gui...
set CURRENT_DIR=%~dp0
set BUILD_MODE=release
set TEST_BINARY=TestGui
set BUILD_FOLDER=temp-tests
set OUTPUT=tests_output.log

echo Checking for test binary... > %OUTPUT%
set TEST_BIN_PATH=%BUILD_FOLDER%\%BUILD_MODE%\%TEST_BINARY%.exe
IF NOT EXIST  %TEST_BIN_PATH% ( 
	echo Test binary does not exist. >> %OUTPUT%
	goto :error 
)

echo Running test binary... >> %OUTPUT%
call %TEST_BIN_PATH% >> %OUTPUT%

IF NOT %errorlevel% == 0 (
	IF %errorlevel% == 1 ( goto :test-failure )
	goto :error
)
popd

echo === Test finished successfully. 
goto :eof
:error
	set FAIL_REASON=%errorlevel%
	echo Test Gui has failed to run correctly. Check binary and libs. (%FAIL_REASON%)
	cd %CURRENT_DIR%
	exit /b %FAIL_REASON%
:test-failure
	set FAIL_REASON=%errorlevel%
	echo At least one failed test case. See %OUTPUT%
	cd %CURRENT_DIR%
	exit /b %FAIL_REASON%