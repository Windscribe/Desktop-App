ECHO off
ECHO ===== Installing jom into c:\libs\jom =====

REM SET VERSION_JOM=1_1_2
rd /s /q "c:\libs\jom"

mkdir c:\libs\jom
curl.exe http://download.qt.io/official_releases/jom/jom_%VERSION_JOM%.zip -o c:\libs\jom\jom_%VERSION_JOM%.zip -k -L
7z x c:\libs\jom\jom_%VERSION_JOM%.zip -oc:\libs\jom
del c:\libs\jom\jom_%VERSION_JOM%.zip

IF NOT EXIST c:\libs\jom\jom.exe (exit /b 1 )

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\jom.zip c:\libs\jom
IF NOT EXIST %2\jom.zip (exit /b 1 )
)
)

exit /b 0