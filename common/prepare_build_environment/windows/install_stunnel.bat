ECHO off
ECHO ===== Installing Stunnel into c:\libs\stunnel =====

REM SET VERSION_STUNNEL=5.58

rd /s /q "c:\stunnel_temp"
rd /s /q "c:\libs\stunnel"

mkdir c:\stunnel_temp
curl.exe https://www.stunnel.org/downloads/stunnel-%VERSION_STUNNEL%.tar.gz -o c:\stunnel_temp\stunnel-%VERSION_STUNNEL%.tar.gz -k -L

7z e c:\stunnel_temp\stunnel-%VERSION_STUNNEL%.tar.gz -oc:\stunnel_temp
7z x c:\stunnel_temp\stunnel-%VERSION_STUNNEL%.tar -oc:\stunnel_temp

copy /Y stunnel_build_file\makew32.bat c:\stunnel_temp\stunnel-%VERSION_STUNNEL%\src\makew32.bat
copy /Y stunnel_build_file\vc.mak c:\stunnel_temp\stunnel-%VERSION_STUNNEL%\src\vc.mak

PUSHD c:\stunnel_temp\stunnel-%VERSION_STUNNEL%\src
call c:\stunnel_temp\stunnel-%VERSION_STUNNEL%\src\makew32.bat


mkdir c:\libs\stunnel
copy c:\stunnel_temp\stunnel-%VERSION_STUNNEL%\bin\win32\tstunnel.exe c:\libs\stunnel\tstunnel.exe

POPD
rd /s /q "c:\stunnel_temp"

IF NOT EXIST c:\libs\stunnel\tstunnel.exe (exit /b 1 )

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\stunnel.zip c:\libs\stunnel
IF NOT EXIST %2\stunnel.zip (exit /b 1 )
)
)
exit /b 0
