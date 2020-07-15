ECHO off
ECHO ===== Installing Stunnel into c:\libs\stunnel =====

rd /s /q "c:\stunnel_temp"
rd /s /q "c:\libs\stunnel"

mkdir c:\stunnel_temp
curl.exe https://www.stunnel.org/downloads/stunnel-5.56.tar.gz -o c:\stunnel_temp\stunnel-5.56.tar.gz -k -L

7z e c:\stunnel_temp\stunnel-5.56.tar.gz -oc:\stunnel_temp
7z x c:\stunnel_temp\stunnel-5.56.tar -oc:\stunnel_temp

copy /Y stunnel_build_file\makew32.bat c:\stunnel_temp\stunnel-5.56\src\makew32.bat
copy /Y stunnel_build_file\vc.mak c:\stunnel_temp\stunnel-5.56\src\vc.mak

PUSHD c:\stunnel_temp\stunnel-5.56\src
call c:\stunnel_temp\stunnel-5.56\src\makew32.bat


mkdir c:\libs\stunnel
copy c:\stunnel_temp\stunnel-5.56\bin\win32\tstunnel.exe c:\libs\stunnel\tstunnel.exe

POPD
rd /s /q "c:\stunnel_temp"

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\stunnel.zip c:\libs\stunnel
)
)
