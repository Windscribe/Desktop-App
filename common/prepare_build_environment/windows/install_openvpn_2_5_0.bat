ECHO off

REM SET VERSION_OPENVPN_25=2.5.0

REM determine major_minor_patch
for /f "tokens=1 delims=." %%i in ("%VERSION_OPENVPN_25%") do SET VERSION_MAJOR=%%i
for /f "tokens=2 delims=." %%i in ("%VERSION_OPENVPN_25%") do SET VERSION_MINOR=%%i
for /f "tokens=3 delims=." %%i in ("%VERSION_OPENVPN_25%") do SET VERSION_PATCH=%%i
SET VERSION_UNDERSCORED=%VERSION_MAJOR%_%VERSION_MINOR%_%VERSION_PATCH%
echo %VERSION_UNDERSCORED%

ECHO ===== Installing openvpn %VERSION_OPENVPN_25% into c:\libs\openvpn_%VERSION_UNDERSCORED% =====


rd /s /q "c:\openvpn_temp"
rd /s /q "c:\libs\openvpn_%VERSION_UNDERSCORED%"

mkdir c:\openvpn_temp

curl.exe https://github.com/OpenVPN/openvpn/archive/v%VERSION_OPENVPN_25%.tar.gz -o c:\openvpn_temp\openvpn-%VERSION_OPENVPN_25%.tar.gz -k -L
7z e c:\openvpn_temp\openvpn-%VERSION_OPENVPN_25%.tar.gz -oc:\openvpn_temp
7z x c:\openvpn_temp\openvpn-%VERSION_OPENVPN_25%.tar -oc:\openvpn_temp
set openvpn_folder="openvpn-%VERSION_OPENVPN_25%"

copy /Y openvpn-%VERSION_OPENVPN_25%_changed_files\version.m4 C:\openvpn_temp\%openvpn_folder%\version.m4
copy /Y openvpn-%VERSION_OPENVPN_25%_changed_files\msvc-build.bat C:\openvpn_temp\%openvpn_folder%\msvc-build.bat
copy /Y openvpn-%VERSION_OPENVPN_25%_changed_files\msvc-env.bat C:\openvpn_temp\%openvpn_folder%\msvc-env.bat
copy /Y openvpn-%VERSION_OPENVPN_25%_changed_files\openvpn.sln C:\openvpn_temp\%openvpn_folder%\openvpn.sln
copy /Y openvpn-%VERSION_OPENVPN_25%_changed_files\propertysheet.props C:\openvpn_temp\%openvpn_folder%\src\compat\propertysheet.props
copy /Y openvpn-%VERSION_OPENVPN_25%_changed_files\compat.vcxproj C:\openvpn_temp\%openvpn_folder%\src\compat\compat.vcxproj
copy /Y openvpn-%VERSION_OPENVPN_25%_changed_files\openvpn.vcxproj C:\openvpn_temp\%openvpn_folder%\src\openvpn\openvpn.vcxproj
copy /Y openvpn-%VERSION_OPENVPN_25%_changed_files\tap-windows.h C:\openvpn_temp\%openvpn_folder%\tap-windows.h
copy /Y openvpn-%VERSION_OPENVPN_25%_changed_files\config-msvc.h C:\openvpn_temp\%openvpn_folder%\config-msvc.h
copy /Y openvpn-%VERSION_OPENVPN_25%_changed_files\tun.h C:\openvpn_temp\%openvpn_folder%\src\openvpn\tun.h

PUSHD c:\openvpn_temp\%openvpn_folder%

call msvc-build.bat

mkdir c:\libs\openvpn_%VERSION_UNDERSCORED%
copy Win32-Output\Release\openvpn.exe c:\libs\openvpn_%VERSION_UNDERSCORED%\openvpn.exe

POPD
rd /s /q "c:\openvpn_temp"

IF NOT EXIST c:\libs\openvpn_%VERSION_UNDERSCORED%\openvpn.exe (exit /b 1 )

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\openvpn_%VERSION_UNDERSCORED%.zip c:\libs\openvpn_%VERSION_UNDERSCORED%
IF NOT EXIST %2\openvpn_%VERSION_UNDERSCORED%.zip (exit /b 1 )
)
)

exit /b 0