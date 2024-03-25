@echo off

set curpath=%cd%
set ARCH_PAR=%1
set FILE_TO_SIGN=%2
set CONTAINER_NAME=%3
set SIGNING_TOKEN_PASSWORD=%4
set VSCOMNTOOLS="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
set INF2CAT_TOOL_PATH="C:\Program Files (x86)\Windows Kits\10\bin\x86\inf2cat.exe"
set AMD_64_OS_LIST=7_X64,8_X64,6_3_X64,10_X64,10_AU_X64,10_RS2_X64,10_RS3_X64,10_RS4_X64,10_RS5_X64,10_19H1_X64,10_VB_X64,Server2008R2_X64,Server8_X64,Server6_3_X64,Server10_X64,Server2016_X64,ServerRS5_X64
set ARM_64_OS_LIST=10_RS3_ARM64,10_RS4_ARM64,10_RS5_ARM64,10_19H1_ARM64,10_VB_ARM64,Server10_ARM64,ServerRS5_ARM64

if NOT exist %VSCOMNTOOLS% (
    echo The Microsoft Visual Studio 2019 vcvarsall.bat not found.
    exit /b
)

if NOT exist %INF2CAT_TOOL_PATH% (
    echo %INF2CAT_TOOL_PATH% not found. Please install Windows DDK.
    exit /b
)

IF NOT "%ARCH_PAR%"=="amd64" (
    IF NOT "%ARCH_PAR%"=="arm64" goto :bad_arch_parameter
)

if NOT exist %curpath%\%ARCH_PAR%\%FILE_TO_SIGN%.sys goto :bad_driver_name_parameter
if NOT exist %curpath%\%ARCH_PAR%\%FILE_TO_SIGN%.inf goto :bad_driver_name_parameter

if "%CONTAINER_NAME%"=="" goto :signing_container_name_missing

if "%SIGNING_TOKEN_PASSWORD%"=="" goto :signing_token_missing

call %VSCOMNTOOLS% x86_amd64


echo Signing %ARCH_PAR% driver with SHA-256...
call signtool.exe sign /v /tr http://timestamp.digicert.com /td sha256 /fd sha256 /f code_signing.der /csp "eToken Base Cryptographic Provider" /kc "[{{%SIGNING_TOKEN_PASSWORD%}}]=%CONTAINER_NAME%" /n "Windscribe Limited" "%curpath%\%ARCH_PAR%\%FILE_TO_SIGN%.sys"
if %errorlevel% equ 0 (
    echo Driver successfully signed.
) ELSE (
    goto :end
)
echo Signing %ARCH_PAR% driver with SHA-1...
call signtool.exe sign /v /as /tr http://timestamp.digicert.com /td sha1 /fd sha1 /f code_signing.der /csp "eToken Base Cryptographic Provider" /kc "[{{%SIGNING_TOKEN_PASSWORD%}}]=%CONTAINER_NAME%" /n "Windscribe Limited" "%curpath%\%ARCH_PAR%\%FILE_TO_SIGN%.sys"
if %errorlevel% equ 0 (
    echo Driver successfully signed.
) ELSE (
    goto :end
)


IF "%ARCH_PAR%"=="amd64" (
    %INF2CAT_TOOL_PATH% /driver:%curpath%/%ARCH_PAR% /os:%AMD_64_OS_LIST%
)
IF "%ARCH_PAR%"=="arm64" (
    %INF2CAT_TOOL_PATH% /driver:%curpath%/%ARCH_PAR% /os:%ARM_64_OS_LIST%
)

echo Signing %ARCH_PAR% catalog file with SHA-256...
call signtool.exe sign /v /tr http://timestamp.digicert.com /td sha256 /fd sha256 /f code_signing.der /csp "eToken Base Cryptographic Provider" /kc "[{{%SIGNING_TOKEN_PASSWORD%}}]=%CONTAINER_NAME%" /n "Windscribe Limited" "%curpath%\%ARCH_PAR%\%FILE_TO_SIGN%.cat"
if %errorlevel% equ 0 (
    echo Driver successfully signed.
) ELSE (
    goto :end
)
echo Signing %ARCH_PAR% catalog file with SHA-1...
call signtool.exe sign /v /as /tr http://timestamp.digicert.com /td sha1 /fd sha1 /f code_signing.der /csp "eToken Base Cryptographic Provider" /kc "[{{%SIGNING_TOKEN_PASSWORD%}}]=%CONTAINER_NAME%" /n "Windscribe Limited" "%curpath%\%ARCH_PAR%\%FILE_TO_SIGN%.cat"
if %errorlevel% equ 0 (
    echo Driver successfully signed.
) ELSE (
    goto :end
)

echo Making cab file...

Set "out=%curpath%"
(
    echo;.OPTION EXPLICIT
    echo;.Set CabinetFileCountThreshold=0
    echo;.Set FolderFileCountThreshold=0
    echo;.Set FolderSizeThreshold=0
    echo;.Set MaxCabinetSize=0
    echo;.Set MaxDiskFileCount=0
    echo;.Set MaxDiskSize=0
    echo;.Set CompressionType=MSZIP
    echo;.Set Cabinet=on
    echo;.Set Compress=on
    echo;.Set CabinetNameTemplate=%FILE_TO_SIGN%_%ARCH_PAR%.cab
    echo;.Set DestinationDir=%ARCH_PAR%
    echo;%ARCH_PAR%\%FILE_TO_SIGN%.inf
    echo;%ARCH_PAR%\%FILE_TO_SIGN%.sys
    echo;%ARCH_PAR%\%FILE_TO_SIGN%.cat
) > "%out%\cabinet_%ARCH_PAR%.ddf"


MakeCab /f cabinet_%ARCH_PAR%.ddf

rem Deleting junk files
del cabinet_%ARCH_PAR%.ddf
del setup.inf
del setup.rpt

echo Signing cabinet file...
call signtool.exe sign /v /tr http://timestamp.digicert.com /td sha256 /fd sha256 /f code_signing.der /csp "eToken Base Cryptographic Provider" /kc "[{{%SIGNING_TOKEN_PASSWORD%}}]=%CONTAINER_NAME%" /n "Windscribe Limited" "%curpath%\disk1\%FILE_TO_SIGN%_%ARCH_PAR%.cab"
if %errorlevel% equ 0 (
    echo Cabinet file successfully signed.
) ELSE (
    goto :end
)

goto :end

:bad_arch_parameter
echo Invalid architecture parameter.
echo Usage: sign.bat [amd64/arm64] driver_name.
goto :end

:bad_driver_name_parameter
echo The driver_name is not specified or driver files not found.
echo Usage: sign.bat [amd64/arm64] driver_name.
goto :end

:signing_container_name_missing
echo The code signing container name was not specified.
goto :end

:signing_token_missing
echo The code signing token password was not specified.
goto :end

:end
