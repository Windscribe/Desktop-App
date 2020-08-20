ECHO off
ECHO ===== Installing WireGuard =====
setlocal enabledelayedexpansion

rd /s /q "c:\wireguard_temp"
rd /s /q "c:\libs\wireguard"

set TOOLSDIR=%~dp0
set BUILDDIR=c:\wireguard_temp\
set WGDIR=wireguard-go-0.0.20200320
set PATH=%BUILDDIR%deps\go\bin;%BUILDDIR%deps\bin;%PATH%

mkdir %BUILDDIR%
pushd %BUILDDIR% || exit /b 1

mkdir deps || goto :error
cd deps || goto :error
call :install go.zip https://dl.google.com/go/go1.14.4.windows-amd64.zip || goto :error
call :install make.zip https://download.wireguard.com/windows-toolchain/distfiles/make-4.2.1-without-guile-w32-bin.zip || goto :error
cd .. || goto :error

call :install wireguard-go.zip https://git.zx2c4.com/wireguard-go/snapshot/%WGDIR%.zip || goto :error
xcopy /E /H /Y %TOOLSDIR%\wireguard_changed_files %BUILDDIR%\%WGDIR% || goto :error

cd %WGDIR%
make || goto :error
mkdir "c:\libs\wireguard"
copy wireguard-go c:\libs\wireguard\wireguard-go.exe || goto :error
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
