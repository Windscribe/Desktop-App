ECHO off
ECHO ===== Installing lzo into c:\libs\lzo =====

REM SET VERSION_LZO=2.10

rd /s /q "c:\lzo_temp"
rd /s /q "c:\libs\lzo"

mkdir c:\lzo_temp
curl.exe http://www.oberhumer.com/opensource/lzo/download/lzo-%VERSION_LZO%.tar.gz -o c:\lzo_temp\lzo-%VERSION_LZO%.tar.gz -k -L
7z e c:\lzo_temp\lzo-%VERSION_LZO%.tar.gz -oc:\lzo_temp
7z x c:\lzo_temp\lzo-%VERSION_LZO%.tar -oc:\lzo_temp

copy /Y lzo_changed_files\vc.bat C:\lzo_temp\lzo-%VERSION_LZO%\B\win32\vc.bat

PUSHD C:\lzo_temp\lzo-%VERSION_LZO%

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
call b\win32\vc.bat


mkdir c:\libs\lzo
mkdir c:\libs\lzo\include
mkdir c:\libs\lzo\lib

copy lzo2.lib c:\libs\lzo\lib\lzo2.lib
xcopy include c:\libs\lzo\include /S /E

POPD
rd /s /q "c:\lzo_temp"

IF NOT EXIST c:\libs\lzo\lib\lzo2.lib (exit /b 1 )

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\lzo.zip c:\libs\lzo
IF NOT EXIST %2\lzo.zip (exit /b 2 )
)
)

exit /b 0