@echo off

echo === Building test binaries...
set CURRENT_DIR=%~dp0
SET VC_VARS_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"
set QMAKE="C:\Qt\bin\qmake.exe"
set JOM="C:\libs\jom\jom.exe"
set BUILD_FOLDER=temp-tests
set GTEST_LIBS="C:\libs\gtest"
set QT="C:\Qt"
set BUILD_MODE="release"

echo === Prepping directories...
IF EXIST %BUILD_FOLDER% ( rd /s /q %BUILD_FOLDER% || goto :error)
mkdir %BUILD_FOLDER% || goto :error
pushd %BUILD_FOLDER% || goto :error

echo === Building Test Gui....
call %VC_VARS_PATH% x86
%QMAKE% ..\..\..\gui\test-gui\TestGui\TestGui.pro -spec win32-msvc "CONFIG+=release"
%JOM% || goto :error

echo === Copying gtest libs...
copy /Y %GTEST_LIBS%\lib\gmock.lib %BUILD_MODE%\gmock.lib
copy /Y %GTEST_LIBS%\lib\gtest.lib %BUILD_MODE%\gtest.lib

echo === Copying other libs...
mkdir %BUILD_MODE%\platforms    || goto :error
mkdir %BUILD_MODE%\imageformats || goto :error
copy /Y %QT%\bin\libEGL.dll 	%BUILD_MODE%\libEGL.dll 	|| goto :error
copy /Y %QT%\bin\libGLESV2.dll  %BUILD_MODE%\libGLESV2.dll  || goto :error
copy /Y %QT%\bin\Qt5Core.dll 	%BUILD_MODE%\Qt5Core.dll 	|| goto :error
copy /Y %QT%\bin\Qt5Gui.dll 	%BUILD_MODE%\Qt5Gui.dll 	|| goto :error
copy /Y %QT%\bin\Qt5Network.dll %BUILD_MODE%\Qt5Network.dll || goto :error
copy /Y %QT%\bin\Qt5Svg.dll  	%BUILD_MODE%\Qt5Svg.dll     || goto :error
copy /Y %QT%\bin\Qt5Widgets.dll %BUILD_MODE%\Qt5Widgets.dll || goto :error
copy /Y %QT%\plugins\platforms\qwindows.dll %BUILD_MODE%\platforms\qwindows.dll || goto :error
copy /Y %QT%\plugins\imageformats\qgif.dll  %BUILD_MODE%\imageformats\qgif.dll  || goto :error
copy /Y %QT%\plugins\imageformats\qico.dll  %BUILD_MODE%\imageformats\qico.dll  || goto :error
copy /Y %QT%\plugins\imageformats\qsvg.dll  %BUILD_MODE%\imageformats\qsvg.dll  || goto :error
copy /Y %QT%\plugins\imageformats\qjpeg.dll %BUILD_MODE%\imageformats\qjpeg.dll || goto :error
popd
popd

echo Successfully built test binaries.
goto :eof
:error
	echo Failed to build test binaries: %errorlevel%
	cd %CURRENT_DIR%
	exit /b %errorlevel%