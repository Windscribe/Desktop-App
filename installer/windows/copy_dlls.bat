echo OFF
SET MSVC_REDIST_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Redist\MSVC\14.16.27012\x86\Microsoft.VC141.CRT"
SET WINKIT_REDIST_PATH="C:\Program Files (x86)\Windows Kits\10\Redist\10.0.17763.0\ucrt\DLLS\x86"
SET QT_LOCATION="C:\Qt"
IF [%1] == [] (
echo You must specify a parameter of output directory
exit /b
)

IF NOT EXIST %MSVC_REDIST_PATH% (
echo MSVC_REDIST_PATH doesn't exist
exit /b
)

IF NOT EXIST %WINKIT_REDIST_PATH% (
echo WINKIT_REDIST_PATH doesn't exist
exit /b
)


set ARG1=%1

copy /Y %QT_LOCATION%\bin\libEGL.dll %ARG1%\libEGL.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y %QT_LOCATION%\bin\libGLESV2.dll %ARG1%\libGLESV2.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y %QT_LOCATION%\bin\Qt5Core.dll %ARG1%\Qt5Core.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y %QT_LOCATION%\bin\Qt5Gui.dll %ARG1%\Qt5Gui.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y %QT_LOCATION%\bin\Qt5Network.dll %ARG1%\Qt5Network.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y %QT_LOCATION%\bin\Qt5Svg.dll %ARG1%\Qt5Svg.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y %QT_LOCATION%\bin\Qt5Widgets.dll %ARG1%\Qt5Widgets.dll
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir %ARG1%\platforms
copy /Y %QT_LOCATION%\plugins\platforms\qwindows.dll %ARG1%\platforms\qwindows.dll
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir %ARG1%\imageformats
copy %QT_LOCATION%\plugins\imageformats\qgif.dll %ARG1%\imageformats\qgif.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y %QT_LOCATION%\plugins\imageformats\qico.dll %ARG1%\imageformats\qico.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y %QT_LOCATION%\plugins\imageformats\qsvg.dll %ARG1%\imageformats\qsvg.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y %QT_LOCATION%\plugins\imageformats\qjpeg.dll %ARG1%\imageformats\qjpeg.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y %MSVC_REDIST_PATH%\concrt140.dll %ARG1%\concrt140.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y %MSVC_REDIST_PATH%\msvcp140.dll %ARG1%\msvcp140.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y %MSVC_REDIST_PATH%\vccorlib140.dll %ARG1%\vccorlib140.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y %MSVC_REDIST_PATH%\vcruntime140.dll %ARG1%\vcruntime140.dll
if %errorlevel% neq 0 exit /b %errorlevel%

xcopy /Y /s additional_files\tap6 %ARG1%\tap6\
xcopy /Y /s additional_files\wintun %ARG1%\wintun\
xcopy /Y /s additional_files\splittunnel %ARG1%\splittunnel\
xcopy /Y /s additional_files\wgsupport\i386 %ARG1%\x32\
xcopy /Y /s additional_files\wgsupport\amd64 %ARG1%\x64\

copy /Y additional_files\wstunnel\wstunnel.exe %ARG1%\wstunnel.exe
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y additional_files\subinacl.exe %ARG1%\subinacl.exe
if %errorlevel% neq 0 exit /b %errorlevel%


copy /Y c:\libs\cares\dll_x32\lib\cares.dll %ARG1%\cares.dll
if %errorlevel% neq 0 exit /b %errorlevel%
copy /Y c:\libs\curl\bin\libcurl.dll %ARG1%\libcurl.dll
if %errorlevel% neq 0 exit /b %errorlevel%
copy /Y c:\libs\openssl\bin\libcrypto-1_1.dll %ARG1%\libcrypto-1_1.dll
if %errorlevel% neq 0 exit /b %errorlevel%
copy /Y c:\libs\openssl\bin\libssl-1_1.dll %ARG1%\libssl-1_1.dll
if %errorlevel% neq 0 exit /b %errorlevel%
copy /Y c:\libs\stunnel\tstunnel.exe %ARG1%\tstunnel.exe
if %errorlevel% neq 0 exit /b %errorlevel%
copy /Y c:\libs\zlib\lib\zlib1.dll %ARG1%\zlib1.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy /Y c:\libs\openvpn_2_5_0\openvpn.exe %ARG1%\windscribeopenvpn_2_5_0.exe
if %errorlevel% neq 0 exit /b %errorlevel%
copy /Y c:\libs\wireguard\windscribewireguard_x86.exe %ARG1%\x32\windscribewireguard.exe
if %errorlevel% neq 0 exit /b %errorlevel%
copy /Y c:\libs\wireguard\windscribewireguard_x64.exe %ARG1%\x64\windscribewireguard.exe
if %errorlevel% neq 0 exit /b %errorlevel%

REM COPY WindowsKits redist
xcopy /Y /s %WINKIT_REDIST_PATH% %ARG1%\

echo Everything ok!
