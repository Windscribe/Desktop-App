ECHO off
ECHO ===== Installing Protobuf into C:\libs\protobuf_release =====

REM NOTE: This script requires cmake be installed first
REM SET VERSION_PROTOBUF=3.14.0

set PROTOBUF_TEMP="c:\protobuf_temp"
set PROTOBUF_TEMP_ZIP="c:\protobuf_temp\protobuf-%VERSION_PROTOBUF%.zip"
set PROTOBUF_INSTALL_LOCATION="C:\libs\protobuf_release"

REM ------ Clear Previously Existing Locations --------

rd /s /q %PROTOBUF_TEMP%
rd /s /q %PROTOBUF_INSTALL_LOCATION%

REM ------ Get Protobuf --------

mkdir %PROTOBUF_TEMP%
curl.exe https://github.com/protocolbuffers/protobuf/archive/v%VERSION_PROTOBUF%.zip -o %PROTOBUF_TEMP_ZIP% -k -L
7z x %PROTOBUF_TEMP_ZIP% -o%PROTOBUF_TEMP%

REM ------ Make & Build --------

pushd C:\protobuf_temp\protobuf-%VERSION_PROTOBUF%\cmake

rd /s /q build

mkdir build
pushd build

rd /s /q release

mkdir release
pushd release

set CMAKE_BINARY="C:\Program Files\CMake\bin\cmake.exe"

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
%CMAKE_BINARY% -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -Dprotobuf_MSVC_STATIC_RUNTIME=OFF -Dprotobuf_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=%PROTOBUF_INSTALL_LOCATION% ../..
nmake install

popd
popd
popd

rd /s /q %PROTOBUF_TEMP%

IF NOT EXIST %PROTOBUF_INSTALL_LOCATION%\bin\protoc.exe (exit /b 1 )

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\protobuf_release.zip c:\libs\protobuf_release
IF NOT EXIST %2\protobuf_release.zip (exit /b 1 )
)
)
exit /b 0