ECHO off
ECHO ===== Installing Python =====

rd /s /q "c:\python_temp"
rd /s /q "c:\python27"

mkdir c:\python_temp
curl.exe https://www.python.org/ftp/python/2.7.18/python-2.7.18.msi -o c:\python_temp\python-2.7.18.msi -k -L
call msiexec /i c:\python_temp\python-2.7.18.msi /quiet /qn /norestart ADDLOCAL=ALL
if %errorlevel% == 3010 ( echo Success: reboot required ) else (if %errorlevel% == 0 ( echo Success ) else ( echo Installation failed with error code %errorlevel% ) )

rd /s /q "c:\python_temp"
