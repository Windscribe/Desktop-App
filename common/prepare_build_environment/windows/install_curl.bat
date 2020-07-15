ECHO off
ECHO ===== Installing Curl into c:\libs\curl =====

rd /s /q "c:\curl_temp"
rd /s /q "c:\libs\curl"

mkdir c:\curl_temp
curl.exe https://curl.haxx.se/download/curl-7.67.0.zip -o c:\curl_temp\curl-7.67.0.zip -k -L
7z x c:\curl_temp\curl-7.67.0.zip -oc:\curl_temp

PUSHD c:\curl_temp\curl-7.67.0\winbuild

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
nmake /f Makefile.vc mode=dll SSL_PATH=c:\libs\openssl ZLIB_PATH=c:\libs\zlib WITH_SSL=dll WITH_ZLIB=dll MACHINE=x86

mkdir c:\libs\curl
xcopy C:\curl_temp\curl-7.67.0\builds\libcurl-vc-x86-release-dll-ssl-dll-zlib-dll-ipv6-sspi c:\libs\curl /S /E

POPD
rd /s /q "c:\curl_temp"

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\curl.zip c:\libs\curl
)
)

