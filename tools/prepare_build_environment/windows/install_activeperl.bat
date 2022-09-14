ECHO off
ECHO ===== Installing Active Perl =====

rd /s /q "c:\perl_temp"

mkdir c:\perl_temp
curl.exe https://downloads.activestate.com/ActivePerl/releases/5.26.3.2603/ActivePerl-5.26.3.2603-MSWin32-x64-a95bce075.exe -o c:\perl_temp\ActivePerl.exe -k -L
call c:\perl_temp\ActivePerl.exe /exenoui /qn /norestart
rd /s /q "c:\perl_temp"
