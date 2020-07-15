ECHO off
ECHO ===== Installing OpenSSL into c:\libs\openssl =====

rd /s /q "c:\openssl_temp"
rd /s /q "c:\libs\openssl"

mkdir c:\openssl_temp
curl.exe https://www.openssl.org/source/openssl-1.1.1d.tar.gz -o c:\openssl_temp\openssl-1.1.1d.tar.gz -k -L

7z e c:\openssl_temp\openssl-1.1.1d.tar.gz -oc:\openssl_temp
7z x c:\openssl_temp\openssl-1.1.1d.tar -oc:\openssl_temp

PUSHD c:\openssl_temp\openssl-1.1.1d

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86

SET PATH=c:\Perl64\bin;%PATH%
perl Configure VC-WIN32 no-asm --prefix=c:\libs\openssl
nmake
nmake install


copy c:\libs\openssl\lib\libcrypto.lib c:\libs\openssl\lib\libeay32.lib
copy c:\libs\openssl\lib\libssl.lib c:\libs\openssl\lib\ssleay32.lib

POPD
rd /s /q "c:\openssl_temp"

REM Handle additional parameters: -zip "PATH" 
IF "%1"=="-zip" (
IF not [%2]==[] (
ECHO "Making zip into %2"
7z.exe a %2\openssl.zip c:\libs\openssl
)
)
