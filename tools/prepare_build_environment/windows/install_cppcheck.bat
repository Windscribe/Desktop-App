ECHO off
ECHO ===== Installing CppCheck =====

rd /s /q "c:\cppcheck_temp"

mkdir c:\cppcheck_temp
curl.exe https://github.com/danmar/cppcheck/releases/download/2.2/cppcheck-2.2-x64-Setup.msi -o c:\cppcheck_temp\cppcheck-2.2-x64-Setup.msi -k -L
call msiexec /i c:\cppcheck_temp\cppcheck-2.2-x64-Setup.msi /passive /norestart

set CPPCHECK_PATH=c:\Program Files\Cppcheck
if exist %CPPCHECK_PATH% (
  echo %PATH% | findstr /i /c:"%CPPCHECK_PATH%" >nul
  if ERRORLEVEL 1 (
    echo Updating PATH environment variable...
    setx PATH "%PATH%;%CPPCHECK_PATH%" /M
  )
)

rd /s /q "c:\cppcheck_temp"
