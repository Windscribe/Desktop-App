@echo off
rem SPDX-License-Identifier: MIT

setlocal
set BUILDDIR=%~dp0
set PATH=%BUILDDIR%.deps\llvm-mingw\bin;%BUILDDIR%.deps\go\bin;%PATH%
set PATHEXT=.exe
cd /d %BUILDDIR% || exit /b 1

if exist .deps\prepared goto :build
:installdeps
	rmdir /s /q .deps 2> NUL
	mkdir .deps || goto :error
	cd .deps || goto :error
	call :download go.zip https://go.dev/dl/go1.25.7.windows-amd64.zip c75e5f4ff62d085cc0017be3ad19d5536f46825fa05db06ec468941f847e3228 || goto :error
	rem llvm-mingw with ARM64 support from https://github.com/mstorsjo/llvm-mingw/releases
	call :download llvm-mingw-msvcrt.zip https://github.com/mstorsjo/llvm-mingw/releases/download/20260127/llvm-mingw-20260127-msvcrt-x86_64.zip 1d9af8ae72bf37574ed695b333bf6bae64f5dbc04d2b480b32b836fc9dae478a || goto :error
	rename llvm-mingw-20260127-msvcrt-x86_64 llvm-mingw || goto :error
	copy /y NUL prepared > NUL || goto :error
	cd .. || goto :error

:build
	set GOOS=windows
	set GOPATH=%BUILDDIR%.deps\gopath
	set GOROOT=%BUILDDIR%.deps\go
	call :build_plat amd64 x86_64 || goto :error
	call :build_plat arm64 aarch64 || goto :error

:success
	echo [+] Success
	exit /b 0

:download
	echo [+] Downloading %1
	curl -#fLo %1 %2 || exit /b 1
	echo [+] Verifying %1
	for /f %%a in ('CertUtil -hashfile %1 SHA256 ^| findstr /r "^[0-9a-f]*$"') do if not "%%a"=="%~3" exit /b 1
	echo [+] Extracting %1
	tar -xf %1 %~4 || exit /b 1
	echo [+] Cleaning up %1
	del %1 || exit /b 1
	goto :eof

:build_plat
	set GOARCH=%~1
	set WINDRES=%~2-w64-mingw32-windres.exe
	mkdir build\%1 >NUL 2>&1
	echo [+] Building version info %1
	%WINDRES% -i wstunnel.rc -O coff -o rsrc.syso || exit /b 1
	echo [+] Building wstunnel %1
	go build -o "build\%1\wstunnel.exe" -a -gcflags=all=-l" "-B -ldflags=-w" "-s || exit /b 1
	goto :eof

:error
	echo [-] Failed with error #%errorlevel%.
	cmd /c exit %errorlevel%
