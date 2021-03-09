echo off

echo Initializing Variables...
set CURRENT_DIR=%~dp0
SET INSTALLER_SOURCES_PATH=%CURRENT_DIR%\..
SET GUI_SOURCES_PATH=%CURRENT_DIR%\..\..\gui
SET BACKEND_SOURCES_PATH=%CURRENT_DIR%\..\..\backend
SET COMMON_SOURCES_PATH=%CURRENT_DIR%\..\..\common
SET VC_VARS_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"
SET MSVC_REDIST_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Redist\MSVC\14.16.27012\x86\Microsoft.VC141.CRT"
SET QMAKE_PATH="C:\Qt\bin\qmake.exe"
SET PYTHON_PATH="c:\Python27\python.exe"
set CERT_PASSWORD="fBafQVi0RC4Ts4zMUFOE"

IF NOT EXIST %MSVC_REDIST_PATH% (
echo MSVC_REDIST_PATH doesn't exist
exit /b
)

IF NOT EXIST %PYTHON_PATH% (
echo PYTHON_PATH doesn't exist
exit /b
)

rem === Extract app version from common\version\windscribe_version.h with python script
FOR /F "tokens=* USEBACKQ" %%F IN (`%PYTHON_PATH% extract_version.py %COMMON_SOURCES_PATH%\version\windscribe_version.h`) DO (
SET APP_VERSION_STRING=%%F
)
echo "App version extracted: %APP_VERSION_STRING%"

IF [%1] == [cp]        (GOTO LabelCopyToEnd)
IF [%1] == [pdb] (GOTO LabelSymbolsToEnd)
IF [%1] == [zip]       (GOTO LabelZipToEnd)
IF [%1] == [sign] 	   (GOTO LabelSignToEnd)
IF [%1] == [installer] (GOTO LabelInstallerAndSign)
IF [%1] == [service]   (
    DEL windscribe.7z
    PUSHD temp-installer
    call %VC_VARS_PATH% x86
    GOTO LabelService
)
rem ==================================================================================
rem === Generating Protobuf ===
echo Generating Protobuf...
call %COMMON_SOURCES_PATH%\ipc\proto\generate_proto.bat

echo
echo

rem GOTO LabelEnd

echo Clearing files and folders from previous build....
DEL windscribe.7z
DEL installer.exe
@RD /S /Q installer-release
@RD /S /Q temp-installer 
mkdir temp-installer
PUSHD temp-installer

echo Setting MSVC...
call %VC_VARS_PATH% x86

rem ==================================================================================
rem === make gui project ===
echo Building Gui...
@RD /S /Q gui-release 
mkdir gui-release
PUSHD gui-release

%QMAKE_PATH% %GUI_SOURCES_PATH%/gui/gui.pro -spec win32-msvc "CONFIG+=release"
C:\libs\jom\jom.exe
POPD

rem ==================================================================================
rem === make cli project ===
echo Building Cli...
@RD /S /Q cli-release
mkdir cli-release
PUSHD cli-release

%QMAKE_PATH% %GUI_SOURCES_PATH%/cli/cli.pro -spec win32-msvc "CONFIG+=release"
C:\libs\jom\jom.exe
POPD

rem ==================================================================================
rem === make backend project ===
echo Building Backend...
@RD /S /Q backend-release 
mkdir backend-release
PUSHD backend-release

%QMAKE_PATH% %BACKEND_SOURCES_PATH%/engine/engine.pro -spec win32-msvc "CONFIG+=release"
C:\libs\jom\jom.exe
POPD

rem ==================================================================================
rem === make service project 32 bits ===
echo Building Service (32)...
@RD /S /Q windscribe-service-release-32 
mkdir windscribe-service-release-32
IF [%1] == [debug] (set ExternalCompilerOptions=/DSKIP_PID_CHECK)
msbuild %BACKEND_SOURCES_PATH%\windows\windscribe_service\windscribe_service.vcxproj /p:OutDir="%cd%/windscribe-service-release-32/" /p:IntermediateOutputPath="%cd%/windscribe-service-release-32/" /p:Configuration=Release


rem ==================================================================================
rem === make changeIcs project 32 bits ===
echo Building changeIcs (32)...
@RD /S /Q change-ics-release-32 
mkdir change-ics-release-32
msbuild %BACKEND_SOURCES_PATH%\windows\changeics\changeics.vcxproj /p:OutDir="%cd%/change-ics-release-32/" /p:IntermediateOutputPath="%cd%/change-ics-release-32/" /p:Configuration=Release

rem ==================================================================================
rem === make WindscribeInstallHelper project ===
echo Building Install Helper...
@RD /S /Q install-helper-release 
mkdir install-helper-release
msbuild %BACKEND_SOURCES_PATH%\windows\windscribe_install_helper\windscribe_install_helper.vcxproj /p:OutDir="%cd%/install-helper-release/" /p:IntermediateOutputPath="%cd%/install-helper-release/" /p:Configuration=Release

rem ==================================================================================
rem === make launcher project ===
echo Building Launcher...
@RD /S /Q launcher-release 
mkdir launcher-release
PUSHD launcher-release
%QMAKE_PATH% %GUI_SOURCES_PATH%\launcher\win\windscribelauncher.pro -spec win32-msvc "CONFIG+=release"
C:\libs\jom\jom.exe
POPD

rem ==================================================================================
rem === make uninstaller project 32 bits ===
echo Building Uninstaller...
@RD /S /Q uninstall-release 
mkdir uninstall-release
msbuild ..\uninstaller\uninstall.vcxproj /p:OutDir="%cd%/uninstall-release/" /p:IntermediateOutputPath="%cd%/uninstall-release/" /p:Configuration=Release

rem ==================================================================================
rem === make service project 64 bits ===
echo Building Service (64)...
@RD /S /Q windscribe-service-release-64 
mkdir windscribe-service-release-64
call %VC_VARS_PATH% x86_amd64
IF [%1] == [debug] (set ExternalCompilerOptions=/DSKIP_PID_CHECK)
msbuild %BACKEND_SOURCES_PATH%\windows\windscribe_service\windscribe_service.vcxproj /p:OutDir="%cd%/windscribe-service-release-64/" /p:IntermediateOutputPath="%cd%/windscribe-service-release-64/" /p:Configuration=Release_x64

rem ==================================================================================
rem === make changeIcs project 64 bits ===
echo Building changeIcs (64)...
@RD /S /Q change-ics-release-64 
mkdir change-ics-release-64
msbuild %BACKEND_SOURCES_PATH%\windows\changeics\changeics.vcxproj /p:OutDir="%cd%/change-ics-release-64/" /p:IntermediateOutputPath="%cd%/change-ics-release-64/" /p:Configuration=Release_x64

rem ==================================================================================
rem === copy all exe ===
echo Moving executables into InstallerFiles...
@RD /S /Q InstallerFiles 
mkdir InstallerFiles
copy gui-release\release\Windscribe.exe InstallerFiles\Windscribe.exe
copy cli-release\release\windscribe-cli.exe InstallerFiles\windscribe-cli.exe
copy backend-release\release\WindscribeEngine.exe InstallerFiles\WindscribeEngine.exe
copy install-helper-release\WindscribeInstallHelper.exe InstallerFiles\WindscribeInstallHelper.exe
copy launcher-release\release\WindscribeLauncher.exe InstallerFiles\WindscribeLauncher.exe
copy uninstall-release\uninstall.exe InstallerFiles\uninstall.exe

mkdir InstallerFiles\x64
mkdir InstallerFiles\x32

copy change-ics-release-32\ChangeIcs.exe InstallerFiles\x32\ChangeIcs.exe
copy change-ics-release-64\ChangeIcs.exe InstallerFiles\x64\ChangeIcs.exe

copy windscribe-service-release-32\WindscribeService.exe InstallerFiles\x32\WindscribeService.exe
copy windscribe-service-release-64\WindscribeService.exe InstallerFiles\x64\WindscribeService.exe

:LabelCopyToEnd

POPD

rem ==================================================================================
rem Copy Dlls and additional files
rem ==================================================================================
echo Moving additional files into InstallerFiles...
call copy_dlls.bat temp-installer\InstallerFiles

:LabelSymbolsToEnd

rem ==================================================================================
rem Copy and pack symbols
rem ==================================================================================
call pack_symbols.bat


:LabelSignToEnd

rem ==================================================================================
rem Signing
rem ==================================================================================
echo Signing...
signing\signtool.exe sign /t http://timestamp.digicert.com /f signing\code_signing.pfx /p %CERT_PASSWORD% temp-installer\InstallerFiles\*.exe
signing\signtool.exe sign /t http://timestamp.digicert.com /f signing\code_signing.pfx /p %CERT_PASSWORD% temp-installer\InstallerFiles\x32\*.exe
signing\signtool.exe sign /t http://timestamp.digicert.com /f signing\code_signing.pfx /p %CERT_PASSWORD% temp-installer\InstallerFiles\x64\*.exe

:LabelZipToEnd

rem ==================================================================================
rem Make 7z archive
rem ==================================================================================
echo Zipping...
additional_files\7z.exe a windscribe.7z .\temp-installer\InstallerFiles\*
 
:LabelInstallerAndSign

rem ==================================================================================
rem === make installer project ===
rem ==================================================================================
echo Building Installer...
@RD /S /Q installer-release 
mkdir installer-release
call %VC_VARS_PATH% x86
msbuild installer\installer.vcxproj /p:OutDir="%cd%/installer-release/" /p:IntermediateOutputPath="%cd%/installer-release/" /p:Configuration=Release

rem ==================================================================================
rem === copy and sign installer ===
rem ==================================================================================
echo Copying and signing installer...
copy /Y installer-release\installer.exe %cd%\Windscribe_%APP_VERSION_STRING%.exe
signing\signtool.exe sign /t http://timestamp.digicert.com /f signing\code_signing.pfx /p %CERT_PASSWORD% Windscribe_%APP_VERSION_STRING%.exe
@RD /S /Q installer-release

rem ==================================================================================
rem === Cleanup
rem @RD /S /Q gui-release 
rem @RD /S /Q backend-release 

:LabelEnd
