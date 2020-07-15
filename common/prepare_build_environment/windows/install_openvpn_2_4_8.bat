ECHO off

ECHO ===== Installing openvpn 2.4.8 into c:\libs\openvpn_2_4_8 =====

rd /s /q "c:\openvpn_temp"
rd /s /q "c:\libs\openvpn_2_4_8"

mkdir c:\openvpn_temp
curl.exe https://github.com/OpenVPN/openvpn/archive/v2.4.8.tar.gz -o c:\openvpn_temp\openvpn-2.4.8.tar.gz -k -L
7z x c:\openvpn_temp\openvpn-2.4.8.zip -oc:\openvpn_temp

7z e c:\openvpn_temp\openvpn-2.4.8.tar.gz -oc:\openvpn_temp
7z x c:\openvpn_temp\openvpn-2.4.8.tar -oc:\openvpn_temp

rem don't forget include APP_LINK c file to openvpn project
copy /Y openvpn-2.4.8_changed_files\version.m4 C:\openvpn_temp\openvpn-2.4.8\version.m4
copy /Y openvpn-2.4.8_changed_files\openvpn.sln C:\openvpn_temp\openvpn-2.4.8\openvpn.sln
copy /Y openvpn-2.4.8_changed_files\compat.vcxproj C:\openvpn_temp\openvpn-2.4.8\src\compat\compat.vcxproj
copy /Y openvpn-2.4.8_changed_files\openvpn.vcxproj C:\openvpn_temp\openvpn-2.4.8\src\openvpn\openvpn.vcxproj
copy /Y openvpn-2.4.8_changed_files\error.c C:\openvpn_temp\openvpn-2.4.8\src\openvpn\error.c
copy /Y openvpn-2.4.8_changed_files\packet_id.h C:\openvpn_temp\openvpn-2.4.8\src\openvpn\packet_id.h
copy /Y openvpn-2.4.8_changed_files\msvc-build.bat C:\openvpn_temp\openvpn-2.4.8\msvc-build.bat
copy /Y openvpn-2.4.8_changed_files\msvc-env.bat C:\openvpn_temp\openvpn-2.4.8\msvc-env.bat
copy /Y openvpn-2.4.8_changed_files\tap-windows.h C:\openvpn_temp\openvpn-2.4.8\tap-windows.h
copy /Y openvpn-2.4.8_changed_files\config-msvc.h C:\openvpn_temp\openvpn-2.4.8\config-msvc.h


PUSHD c:\openvpn_temp\openvpn-2.4.8

call msvc-build.bat

mkdir c:\libs\openvpn_2_4_8
copy Win32-Output\Release\openvpn.exe c:\libs\openvpn_2_4_8\openvpn.exe

POPD
rd /s /q "c:\openvpn_temp"

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\openvpn_2_4_8.zip c:\libs\openvpn_2_4_8
)
)

