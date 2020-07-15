ECHO off
ECHO ===== Installing zlib into c:\libs\zlib =====

rd /s /q "c:\zlib_temp"
rd /s /q "c:\libs\zlib"

mkdir c:\zlib_temp
curl.exe https://github.com/madler/zlib/archive/v1.2.11.zip -o c:\zlib_temp\v1.2.11.zip -k -L
7z x c:\zlib_temp\v1.2.11.zip -oc:\zlib_temp

PUSHD c:\zlib_temp\zlib-1.2.11

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
nmake -f win32/Makefile.msc


mkdir c:\libs\zlib\include
mkdir c:\libs\zlib\lib

copy zconf.h c:\libs\zlib\include\zconf.h
copy zlib.h c:\libs\zlib\include\zlib.h

copy zdll.exp c:\libs\zlib\lib\zdll.exp
copy zdll.lib c:\libs\zlib\lib\zdll.lib
copy zlib1.dll c:\libs\zlib\lib\zlib1.dll

POPD
rd /s /q "c:\zlib_temp"

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\zlib.zip c:\libs\zlib
)
)
