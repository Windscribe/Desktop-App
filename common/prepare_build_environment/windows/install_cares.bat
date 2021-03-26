ECHO off
ECHO ===== Installing C-Ares into c:\libs\cares =====

REM SET VERSION_CARES=1.17.1
rd /s /q "c:\cares_temp"
rd /s /q "c:\libs\cares"

ECHO ===== Building C-Ares 32 (Dll) =====

mkdir c:\cares_temp
curl.exe https://c-ares.haxx.se/download/c-ares-%VERSION_CARES%.tar.gz -o c:\cares_temp\c-ares-%VERSION_CARES%.tar.gz -k -L

7z e c:\cares_temp\c-ares-%VERSION_CARES%.tar.gz -oc:\cares_temp
7z x c:\cares_temp\c-ares-%VERSION_CARES%.tar -oc:\cares_temp

PUSHD c:\cares_temp\c-ares-%VERSION_CARES%
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86

nmake -f Makefile.msvc  
SET INSTALL_DIR_DLL_32=c:\libs\cares\dll_x32
set INSTALL_DIR=%INSTALL_DIR_DLL_32%
nmake -f Makefile.msvc install                

POPD
rd /s /q "c:\cares_temp"

ECHO ===== Building C-Ares 64 (Dll) =====

mkdir c:\cares_temp
curl.exe https://c-ares.haxx.se/download/c-ares-%VERSION_CARES%.tar.gz -o c:\cares_temp\c-ares-%VERSION_CARES%.tar.gz -k -L

7z e c:\cares_temp\c-ares-%VERSION_CARES%.tar.gz -oc:\cares_temp
7z x c:\cares_temp\c-ares-%VERSION_CARES%.tar -oc:\cares_temp

PUSHD c:\cares_temp\c-ares-%VERSION_CARES%
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64

nmake -f Makefile.msvc 
SET INSTALL_DIR_DLL_64=c:\libs\cares\dll_x64
set INSTALL_DIR=%INSTALL_DIR_DLL_64%
nmake -f Makefile.msvc install                

POPD
rd /s /q "c:\cares_temp"

ECHO ===== Building C-Ares 32 (Static) =====
mkdir c:\cares_temp
curl.exe https://c-ares.haxx.se/download/c-ares-%VERSION_CARES%.tar.gz -o c:\cares_temp\c-ares-%VERSION_CARES%.tar.gz -k -L

7z e c:\cares_temp\c-ares-%VERSION_CARES%.tar.gz -oc:\cares_temp
7z x c:\cares_temp\c-ares-%VERSION_CARES%.tar -oc:\cares_temp

copy /Y cares_changed_files\Makefile.msvc C:\cares_temp\c-ares-%VERSION_CARES%\Makefile.msvc

PUSHD c:\cares_temp\c-ares-%VERSION_CARES%
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86

nmake -f Makefile.msvc  
SET INSTALL_DIR_STATIC_32=c:\libs\cares\static_x32
set INSTALL_DIR=%INSTALL_DIR_STATIC_32%
nmake -f Makefile.msvc install                

POPD
rd /s /q "c:\cares_temp"

ECHO ===== Building C-Ares 64 (Static) =====

mkdir c:\cares_temp
curl.exe https://c-ares.haxx.se/download/c-ares-%VERSION_CARES%.tar.gz -o c:\cares_temp\c-ares-%VERSION_CARES%.tar.gz -k -L

7z e c:\cares_temp\c-ares-%VERSION_CARES%.tar.gz -oc:\cares_temp
7z x c:\cares_temp\c-ares-%VERSION_CARES%.tar -oc:\cares_temp

copy /Y cares_changed_files\Makefile.msvc C:\cares_temp\c-ares-%VERSION_CARES%\Makefile.msvc

PUSHD c:\cares_temp\c-ares-%VERSION_CARES%
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64

nmake -f Makefile.msvc   
set INSTALL_DIR_STATIC_64=c:\libs\cares\static_x64
set INSTALL_DIR=%INSTALL_DIR_STATIC_64%
nmake -f Makefile.msvc install                

POPD
rd /s /q "c:\cares_temp"

IF NOT EXIST %INSTALL_DIR_DLL_32%\lib\cares.dll (exit /b 1 )
IF NOT EXIST %INSTALL_DIR_DLL_64%\lib\cares.dll (exit /b 1 )
IF NOT EXIST %INSTALL_DIR_STATIC_32%\lib\cares.dll (exit /b 1 )
IF NOT EXIST %INSTALL_DIR_STATIC_64%\lib\cares.dll (exit /b 1 )

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\cares.zip c:\libs\cares
IF NOT EXIST %2\cares.zip (exit /b 2 )
)
)

exit /b 0