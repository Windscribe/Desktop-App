ECHO off
ECHO ===== Installing Protobuf into C:\libs\protobuf_debug=====

REM NOTE: This script requires cmake be installed first

set PROTOBUF_TEMP="c:\protobuf_temp"
set PROTOBUF_TEMP_ZIP="c:\protobuf_temp\protobuf-3.8.0.zip"
set PROTOBUF_INSTALL_LOCATION="C:\libs\protobuf_debug"

REM ------ Clear Previously Existing Locations --------

rd /s /q %PROTOBUF_TEMP%
rd /s /q %PROTOBUF_INSTALL_LOCATION%

REM ------ Get Protobuf --------

mkdir %PROTOBUF_TEMP%
curl.exe https://github.com/protocolbuffers/protobuf/archive/v3.8.0.zip -o %PROTOBUF_TEMP_ZIP% -k -L
7z x %PROTOBUF_TEMP_ZIP% -o%PROTOBUF_TEMP%

REM ------ Make & Build --------

pushd C:\protobuf_temp\protobuf-3.8.0\cmake

rd /s /q build

mkdir build
pushd build

rd /s /q debug

mkdir debug
pushd debug

set CMAKE_BINARY="C:\Program Files\CMake\bin\cmake.exe"

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
%CMAKE_BINARY% -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug -Dprotobuf_MSVC_STATIC_RUNTIME=OFF -Dprotobuf_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=%PROTOBUF_INSTALL_LOCATION% ../..
nmake install

popd
popd
popd

rd /s /q %PROTOBUF_TEMP%


REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\protobuf_debug.zip c:\libs\protobuf_debug
)
)
