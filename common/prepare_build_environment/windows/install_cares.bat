ECHO off
ECHO ===== Installing C-Ares into c:\libs\cares =====

rd /s /q "c:\cares_temp"
rd /s /q "c:\libs\cares"


ECHO ===== Building C-Ares 32 (Dll) =====

mkdir c:\cares_temp
curl.exe https://c-ares.haxx.se/download/c-ares-1.15.0.tar.gz -o c:\cares_temp\c-ares-1.15.0.tar.gz -k -L

7z e c:\cares_temp\c-ares-1.15.0.tar.gz -oc:\cares_temp
7z x c:\cares_temp\c-ares-1.15.0.tar -oc:\cares_temp

PUSHD c:\cares_temp\c-ares-1.15.0
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86

nmake -f Makefile.msvc   
set INSTALL_DIR=c:\libs\cares\dll_x32
nmake -f Makefile.msvc install                

POPD
rd /s /q "c:\cares_temp"


ECHO ===== Building C-Ares 64 (Dll) =====

mkdir c:\cares_temp
curl.exe https://c-ares.haxx.se/download/c-ares-1.15.0.tar.gz -o c:\cares_temp\c-ares-1.15.0.tar.gz -k -L

7z e c:\cares_temp\c-ares-1.15.0.tar.gz -oc:\cares_temp
7z x c:\cares_temp\c-ares-1.15.0.tar -oc:\cares_temp

PUSHD c:\cares_temp\c-ares-1.15.0
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64

nmake -f Makefile.msvc   
set INSTALL_DIR=c:\libs\cares\dll_x64
nmake -f Makefile.msvc install                

POPD
rd /s /q "c:\cares_temp"


ECHO ===== Building C-Ares 32 (Static) =====

mkdir c:\cares_temp
curl.exe https://c-ares.haxx.se/download/c-ares-1.15.0.tar.gz -o c:\cares_temp\c-ares-1.15.0.tar.gz -k -L

7z e c:\cares_temp\c-ares-1.15.0.tar.gz -oc:\cares_temp
7z x c:\cares_temp\c-ares-1.15.0.tar -oc:\cares_temp

copy /Y cares_changed_files\Makefile.msvc C:\cares_temp\c-ares-1.15.0\Makefile.msvc

PUSHD c:\cares_temp\c-ares-1.15.0
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86

nmake -f Makefile.msvc   
set INSTALL_DIR=c:\libs\cares\static_x32
nmake -f Makefile.msvc install                

POPD
rd /s /q "c:\cares_temp"


ECHO ===== Building C-Ares 64 (Static) =====

mkdir c:\cares_temp
curl.exe https://c-ares.haxx.se/download/c-ares-1.15.0.tar.gz -o c:\cares_temp\c-ares-1.15.0.tar.gz -k -L

7z e c:\cares_temp\c-ares-1.15.0.tar.gz -oc:\cares_temp
7z x c:\cares_temp\c-ares-1.15.0.tar -oc:\cares_temp

copy /Y cares_changed_files\Makefile.msvc C:\cares_temp\c-ares-1.15.0\Makefile.msvc

PUSHD c:\cares_temp\c-ares-1.15.0
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64

nmake -f Makefile.msvc   
set INSTALL_DIR=c:\libs\cares\static_x64
nmake -f Makefile.msvc install                

POPD
rd /s /q "c:\cares_temp"

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\cares.zip c:\libs\cares
)
)
