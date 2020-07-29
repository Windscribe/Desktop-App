Wintun driver sources.

1. Downloaded from git (version 0.8).
2. Changes have been applied to make 32-bit openvpn executable work with 64-bit wintun driver.
3. Use winmerge for check what changes.



The workflow for signing driver (for Windows 7 - 10):

- Build windtun420.sys
- SHA-256 sign windtun420.sys
  signtool sign /v /ac "c:\wintun-0.8\DigiCert High Assurance EV Root CA.crt" /tr http://timestamp.digicert.com /td sha256 /fd sha256 /s my /n "Windscribe Limited" "windtun420.sys"

- append SHA-1 signature to wintun.sys
  signtool sign /v /as /ac "c:\wintun-0.8\DigiCert High Assurance EV Root CA.crt" /tr http://timestamp.digicert.com /td sha1 /fd sha1 /s my /n "Windscribe Limited" "windtun420.sys"

- Inf2Cat
   x64) Inf2Cat /driver:C:\wintun-0.8\amd64\Release\wintun /os:7_X64,8_X64,6_3_X64,10_X64,10_AU_X64,10_RS2_X64,10_RS3_X64,10_RS4_X64,Server2008R2_X64,Server8_X64,Server6_3_X64,Server10_X64,Server2016_X64,ServerRS2_X64,ServerRS3_X64,ServerRS4_X64
   x32) Inf2Cat /driver:C:\wintun-0.8\x86\Release\wintun /os:7_X86,8_X86,6_3_X86,10_X86,10_AU_X86,10_RS2_X86,10_RS3_X86,10_RS4_X86



- SHA-256 sign wintun.cat
- append SHA-1 signature to wintun.cat

Signing with scripts.
1. Copy windtun420.sys and windtun420.inf (both x64 and x32 versions) to "signing\i386" and "signing\amd64" folders.
2. Copy "signing" folder to the signing remote computer.
3. Run sign.bat script.

As a result of these steps we get drivers for Windows 7.
For Windows 10, make additional steps:
1. Run make_cab.bat
2. Send cab-file to Microsoft.
3. Get signed driver files from Microsoft.





