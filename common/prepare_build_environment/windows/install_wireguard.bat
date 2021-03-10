ECHO off
ECHO ===== Installing WireGuard =====
setlocal enabledelayedexpansion

rd /s /q "c:\wireguard_temp"
rd /s /q "c:\libs\wireguard"

REM SET VERSION_GO=1.14.4
REM SET VERSION_WIREGUARD=4.2.1

set TOOLSDIR=%~dp0
set BUILDDIR=c:\wireguard_temp\
set WGDIR=wireguard-go-0.0.20201118
set PATH=%BUILDDIR%deps\go\bin;%BUILDDIR%deps\bin;%PATH%

mkdir %BUILDDIR%
pushd %BUILDDIR% || exit /b 1

mkdir deps || goto :error
cd deps || goto :error
call :install go.zip https://dl.google.com/go/go%VERSION_GO%.windows-amd64.zip || goto :error
call :install make.zip https://download.wireguard.com/windows-toolchain/distfiles/make-%VERSION_WIREGUARD%-without-guile-w32-bin.zip || goto :error
cd .. || goto :error

call :install wireguard-go.zip https://git.zx2c4.com/wireguard-go/snapshot/%WGDIR%.zip || goto :error
xcopy /E /H /Y %TOOLSDIR%\wireguard_changed_files %BUILDDIR%\%WGDIR% || goto :error

cd %WGDIR%
make || goto :error
mkdir "c:\libs\wireguard"
copy wireguard-go c:\libs\wireguard\windscribewireguard_x64.exe || goto :error
del wireguard-go
set GOARCH=386
make || goto :error
copy wireguard-go c:\libs\wireguard\windscribewireguard_x86.exe || goto :error
popd
rd /s /q "c:\wireguard_temp"
goto :eof

:install
	echo Downloading %1
	curl -#fLo "%1" "%2" || exit /b 1
	echo Extracting %1
	%TOOLSDIR%\7z x "%1" || exit /b 1
	echo Cleaning up %1
	del "%1" || exit /b 1
	goto :eof

:error
	echo [-] Failed with error #%errorlevel%.
	cmd /c exit %errorlevel%
