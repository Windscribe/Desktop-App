@echo off

set curpath=%cd%
set INF2CAT_TOOL_PATH="C:\Program Files (x86)\Windows Kits\10\bin\x86\inf2cat.exe" 

set VSCOMNTOOLS="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"
call %VSCOMNTOOLS% x86_amd64

echo "Signing architecture amd64"
signtool sign /v /ac "DigiCert High Assurance EV Root CA.crt" /tr http://timestamp.digicert.com /td sha256 /fd sha256 /s my /n "Windscribe Limited" "%curpath%\amd64\windtun420.sys"
signtool sign /v /as /ac "DigiCert High Assurance EV Root CA.crt" /tr http://timestamp.digicert.com /td sha1 /fd sha1 /s my /n "Windscribe Limited" "%curpath%\amd64\windtun420.sys"

%INF2CAT_TOOL_PATH% /driver:%curpath%/amd64 /os:7_X64,8_X64,6_3_X64,10_X64,10_AU_X64,10_RS2_X64,10_RS3_X64,10_RS4_X64,Server2008R2_X64,Server8_X64,Server6_3_X64,Server10_X64,Server2016_X64,ServerRS2_X64,ServerRS3_X64,ServerRS4_X64

signtool sign /v /ac "DigiCert High Assurance EV Root CA.crt" /tr http://timestamp.digicert.com /td sha256 /fd sha256 /s my /n "Windscribe Limited" "%curpath%\amd64\windtun420.cat"
signtool sign /v /as /ac "DigiCert High Assurance EV Root CA.crt" /tr http://timestamp.digicert.com /td sha1 /fd sha1 /s my /n "Windscribe Limited" "%curpath%\amd64\windtun420.cat"

echo "Signing architecture i386"
signtool sign /v /ac "DigiCert High Assurance EV Root CA.crt" /tr http://timestamp.digicert.com /td sha256 /fd sha256 /s my /n "Windscribe Limited" "%curpath%\i386\windtun420.sys"
signtool sign /v /as /ac "DigiCert High Assurance EV Root CA.crt" /tr http://timestamp.digicert.com /td sha1 /fd sha1 /s my /n "Windscribe Limited" "%curpath%\i386\windtun420.sys"

%INF2CAT_TOOL_PATH% /driver:%curpath%/i386 /os:7_X86,8_X86,6_3_X86,10_X86,10_AU_X86,10_RS2_X86,10_RS3_X86,10_RS4_X86

signtool sign /v /ac "DigiCert High Assurance EV Root CA.crt" /tr http://timestamp.digicert.com /td sha256 /fd sha256 /s my /n "Windscribe Limited" "%curpath%\i386\windtun420.cat"
signtool sign /v /as /ac "DigiCert High Assurance EV Root CA.crt" /tr http://timestamp.digicert.com /td sha1 /fd sha1 /s my /n "Windscribe Limited" "%curpath%\i386\windtun420.cat"

