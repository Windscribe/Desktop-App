ECHO off
ECHO ===== Installing jom into c:\libs\jom =====

rd /s /q "c:\libs\jom"

mkdir c:\libs\jom
curl.exe http://download.qt.io/official_releases/jom/jom_1_1_2.zip -o c:\libs\jom\jom_1_1_2.zip -k -L
7z x c:\libs\jom\jom_1_1_2.zip -oc:\libs\jom
del c:\libs\jom\jom_1_1_2.zip

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\jom.zip c:\libs\jom
)
)
