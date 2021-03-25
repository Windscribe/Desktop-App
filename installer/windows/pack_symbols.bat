@echo off

rem echo Initializing Variables...
set CURRENT_DIR=%~dp0
set COMMON_SOURCES_PATH=%CURRENT_DIR%..\..\common
set PYTHON_PATH="c:\Python27\python.exe"
set BUILD_PATH=%CURRENT_DIR%\temp-installer
set TEMP_SYMBOLS_PATH=%BUILD_PATH%\symbols

if not exist %PYTHON_PATH% (
	echo PYTHON_PATH doesn't exist
	goto :error
)

SET WS_VERSION_FILE=%COMMON_SOURCES_PATH%\version\windscribe_version.h
IF NOT EXIST %WS_VERSION_FILE% ( 
	echo Couldn't find version file: %WS_VERSION_FILE%
	goto :error 
)

rem === Extract app version from client-desktop\common\version\windscribe_version.h with python script
for /F "tokens=* USEBACKQ" %%F IN (`%PYTHON_PATH% extract_version.py %WS_VERSION_FILE%`) DO (
  set APP_VERSION_STRING=%%F
) 
echo App version extracted: %APP_VERSION_STRING%

echo Packing symbols...
rd /S /Q %TEMP_SYMBOLS_PATH%
mkdir %TEMP_SYMBOLS_PATH%		|| goto :error
mkdir %TEMP_SYMBOLS_PATH%\x64	|| goto :error
copy %BUILD_PATH%\backend-release\release\WindscribeEngine.pdb %TEMP_SYMBOLS_PATH%\  			|| goto :error
copy %BUILD_PATH%\gui-release\release\Windscribe.pdb %TEMP_SYMBOLS_PATH%\						|| goto :error
copy %BUILD_PATH%\cli-release\release\windscribe-cli.pdb %TEMP_SYMBOLS_PATH%\					|| goto :error
copy %BUILD_PATH%\install-helper-release\WindscribeInstallHelper.pdb %TEMP_SYMBOLS_PATH%\		|| goto :error
copy %BUILD_PATH%\change-ics-release-32\ChangeIcs.pdb %TEMP_SYMBOLS_PATH%\						|| goto :error
copy %BUILD_PATH%\windscribe-service-release-32\WindscribeService.pdb %TEMP_SYMBOLS_PATH%\		|| goto :error
copy %BUILD_PATH%\change-ics-release-64\ChangeIcs.pdb %TEMP_SYMBOLS_PATH%\x64\					|| goto :error
copy %BUILD_PATH%\windscribe-service-release-64\WindscribeService.pdb %TEMP_SYMBOLS_PATH%\x64\	|| goto :error

set SYMBOL_ARCHIVE_NAME=WindscribeSymbols_%APP_VERSION_STRING%.7z
if exist %SYMBOL_ARCHIVE_NAME% del %SYMBOL_ARCHIVE_NAME%
additional_files\7z.exe a %SYMBOL_ARCHIVE_NAME% %TEMP_SYMBOLS_PATH%\* || goto :error

echo Packing symbols complete!
goto :eof

:error
	echo Failed to pack symbols.
	exit /b %errorlevel%