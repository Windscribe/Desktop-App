ECHO off
ECHO ===== Installing Python =====

rd /s /q "c:\python_temp"
rd /s /q "c:\python27"

mkdir c:\python_temp
curl.exe https://www.python.org/ftp/python/2.7.14/python-2.7.14.msi -o c:\python_temp\python-2.7.14.msi -k -L
call msiexec /i c:\python_temp\python-2.7.14.msi /quiet /qn /norestart

rd /s /q "c:\python_temp"
