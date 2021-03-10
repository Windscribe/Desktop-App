ECHO off
ECHO ===== Installing boost into c:\libs\boost =====

REM SET VERSION_BOOST=1.67.0

REM determine major_minor_patch
for /f "tokens=1 delims=." %%i in ("%VERSION_BOOST%") do SET VERSION_MAJOR=%%i
for /f "tokens=2 delims=." %%i in ("%VERSION_BOOST%") do SET VERSION_MINOR=%%i
for /f "tokens=3 delims=." %%i in ("%VERSION_BOOST%") do SET VERSION_PATCH=%%i
SET VERSION_BOOST_UNDERSCORED=%VERSION_MAJOR%_%VERSION_MINOR%_%VERSION_PATCH%
echo %VERSION_BOOST_UNDERSCORED%

rd /s /q "c:\boost_temp"
rd /s /q "c:\libs\boost"

mkdir c:\boost_temp
curl.exe https://dl.bintray.com/boostorg/release/%VERSION_BOOST%/source/boost_%VERSION_BOOST_UNDERSCORED%.zip -o c:\boost_temp\boost_%VERSION_BOOST_UNDERSCORED%.zip -k -L

7z x c:\boost_temp\boost_%VERSION_BOOST_UNDERSCORED%.zip -oc:\boost_temp

PUSHD c:\boost_temp\boost_%VERSION_BOOST_UNDERSCORED%
call bootstrap.bat
b2 install --build-type=complete --prefix=c:\libs\boost || exit /b 1

POPD
rd /s /q "c:\boost_temp"

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\boost.zip c:\libs\boost
IF NOT EXIST %2\boost.zip (exit /b 2 )
)
)

exit /b 0