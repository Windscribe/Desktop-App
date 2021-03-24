@echo OFF

set SCRIPT_DIR=%~dp0
SET GTEST_VERSION=1.10.0
set CMAKE="C:\Program Files\CMake\bin\cmake.exe"
set VC_VARSALL="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"
set LIBS_GTEST_RELEASE=C:\libs\gtest
set TEMP_FOLDER=C:\gtest_temp

echo ===== Cleaning up system libs and temp folder...
IF EXIST %LIBS_GTEST_RELEASE% ( rd /s /q %LIBS_GTEST_RELEASE% || goto :error )
IF EXIST %TEMP_FOLDER% ( rd /s /q %TEMP_FOLDER% || goto :error )

echo ===== Creating temp folder... %SCRIPT_DIR%
mkdir %TEMP_FOLDER%
curl.exe https://github.com/google/googletest/archive/release-%GTEST_VERSION%.zip -o c:\gtest_temp\gtest.zip -k -L
7z x %TEMP_FOLDER%\gtest.zip -oc:\gtest_temp

echo ===== Navigating to build dir
pushd %TEMP_FOLDER%\googletest-release-%GTEST_VERSION% || goto :error
mkdir build || goto :error
pushd build || goto :error

echo ===== Prepping makefiles for dynamic linking...
%CMAKE% ..  -Dgtest_force_shared_crt=ON || goto :error

echo ===== Building Google Test...
call %VC_VARSALL% x86 || goto :error
msbuild googletest-distribution.sln /p:Configuration=Release /p:Platform="Win32" || goto :error

echo ===== Moving libs to system libs...
IF NOT EXIST %LIBS_GTEST_RELEASE%\lib ( mkdir %LIBS_GTEST_RELEASE%\lib || goto :error)
xcopy /Y /s lib\Release %LIBS_GTEST_RELEASE%\lib || goto :error
popd

echo ===== Moving header files to system libs...
IF NOT EXIST %LIBS_GTEST_RELEASE%\include ( mkdir %LIBS_GTEST_RELEASE%\include || goto :error )
xcopy /Y /s googlemock\include %LIBS_GTEST_RELEASE%\include || goto :error
IF NOT EXIST %LIBS_GTEST_RELEASE%\include ( mkdir %LIBS_GTEST_RELEASE%\include || goto :error )
xcopy /Y /s googletest\include %LIBS_GTEST_RELEASE%\include || goto :error
popd

echo ===== Removing temp....
rd /s /q %TEMP_FOLDER% 

echo Successfully installed gtest.
goto :eof
:error
	set FAIL_REASON=%errorlevel%
	echo Failed to install gtest: %FAIL_REASON%
	cd %SCRIPT_DIR%
	exit /b %FAIL_REASON%