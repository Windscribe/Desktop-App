ECHO off

ECHO ===== Installing openvpn 2.5.0 into c:\libs\openvpn_2_5_0=====

rd /s /q "c:\openvpn_temp"
rd /s /q "c:\libs\openvpn_2_5_0"

mkdir c:\openvpn_temp
curl.exe https://github.com/OpenVPN/openvpn/archive/master.zip -o c:\openvpn_temp\openvpn-2.5.0.zip -k -L
7z x c:\openvpn_temp\openvpn-2.5.0.zip -oc:\openvpn_temp

copy /Y openvpn-2.5.0_changed_files\version.m4 C:\openvpn_temp\openvpn-master\version.m4
copy /Y openvpn-2.5.0_changed_files\msvc-build.bat C:\openvpn_temp\openvpn-master\msvc-build.bat
copy /Y openvpn-2.5.0_changed_files\msvc-env.bat C:\openvpn_temp\openvpn-master\msvc-env.bat
copy /Y openvpn-2.5.0_changed_files\openvpn.sln C:\openvpn_temp\openvpn-master\openvpn.sln
copy /Y openvpn-2.5.0_changed_files\propertysheet.props C:\openvpn_temp\openvpn-master\src\compat\propertysheet.props
copy /Y openvpn-2.5.0_changed_files\compat.vcxproj C:\openvpn_temp\openvpn-master\src\compat\compat.vcxproj
copy /Y openvpn-2.5.0_changed_files\openvpn.vcxproj C:\openvpn_temp\openvpn-master\src\openvpn\openvpn.vcxproj
copy /Y openvpn-2.5.0_changed_files\tap-windows.h C:\openvpn_temp\openvpn-master\tap-windows.h
copy /Y openvpn-2.5.0_changed_files\config-msvc.h C:\openvpn_temp\openvpn-master\config-msvc.h


PUSHD c:\openvpn_temp\openvpn-master

call msvc-build.bat

mkdir c:\libs\openvpn_2_5_0
copy Win32-Output\Release\openvpn.exe c:\libs\openvpn_2_5_0\openvpn.exe

POPD
rd /s /q "c:\openvpn_temp"


REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\openvpn_2_5_0.zip c:\libs\openvpn_2_5_0
)
)
