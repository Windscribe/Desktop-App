@echo off
MakeCab /f cabinet_amd64.ddf
MakeCab /f cabinet_i386.ddf

set curpath=%cd%

set VSCOMNTOOLS="C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"
call %VSCOMNTOOLS% x86_amd64

echo "Signing cab file"
signtool sign /v /ac "DigiCert High Assurance EV Root CA.crt" /tr http://timestamp.digicert.com /td sha256 /fd sha256 /s my /n "Windscribe Limited" "%curpath%\disk1\windtun420_amd64.cab"
signtool sign /v /ac "DigiCert High Assurance EV Root CA.crt" /tr http://timestamp.digicert.com /td sha256 /fd sha256 /s my /n "Windscribe Limited" "%curpath%\disk1\windtun420_i386.cab"
