ECHO off
ECHO ===== Installing boost into c:\libs\boost =====

rd /s /q "c:\boost_temp"
rd /s /q "c:\libs\boost"

mkdir c:\boost_temp
curl.exe https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.zip -o c:\boost_temp\boost_1_67_0.zip -k -L

7z x c:\boost_temp\boost_1_67_0.zip -oc:\boost_temp

PUSHD c:\boost_temp\boost_1_67_0
call bootstrap.bat
b2 install --build-type=complete --prefix=c:\libs\boost

POPD
rd /s /q "c:\boost_temp"

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\boost.zip c:\libs\boost
)
)