echo OFF
SET MSVC_REDIST_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Redist\MSVC\14.15.26706\x86\Microsoft.VC141.CRT"

IF NOT EXIST %MSVC_REDIST_PATH% (
echo MSVC_REDIST_PATH doesn't exist
exit /b
)

rd /s /q "temp"
del windscribe.7z

mkdir temp

copy c:\Qt\bin\libEGL.dll temp\libEGL.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy c:\Qt\bin\libGLESV2.dll temp\libGLESV2.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy c:\Qt\bin\Qt5Core.dll temp\Qt5Core.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy c:\Qt\bin\Qt5Gui.dll temp\Qt5Gui.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy c:\Qt\bin\Qt5Network.dll temp\Qt5Network.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy c:\Qt\bin\Qt5Svg.dll temp\Qt5Svg.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy c:\Qt\bin\Qt5Widgets.dll temp\Qt5Widgets.dll
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir temp\platforms
copy c:\Qt\plugins\platforms\qwindows.dll temp\platforms\qwindows.dll
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir temp\imageformats
copy c:\Qt\plugins\imageformats\qgif.dll temp\imageformats\qgif.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy c:\Qt\plugins\imageformats\qico.dll temp\imageformats\qico.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy c:\Qt\plugins\imageformats\qsvg.dll temp\imageformats\qsvg.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy %MSVC_REDIST_PATH%\concrt140.dll temp\concrt140.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy %MSVC_REDIST_PATH%\msvcp140.dll temp\msvcp140.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy %MSVC_REDIST_PATH%\vccorlib140.dll temp\vccorlib140.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy %MSVC_REDIST_PATH%\vcruntime140.dll temp\vcruntime140.dll
if %errorlevel% neq 0 exit /b %errorlevel%

copy exe\WindscribeGUI.exe temp\WindscribeGUI.exe
if %errorlevel% neq 0 exit /b %errorlevel%

7z.exe a windscribe.7z .\temp\*

echo Everything ok!
