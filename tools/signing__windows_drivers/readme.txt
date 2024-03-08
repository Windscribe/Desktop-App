The script sign.bat is designed to sign drivers for Windows.
Currently supports 2 architectures amd64 and arm64.

The signing process consists of two steps:

Step1 (signing and creating an cabinet file ready for upload to the Microsoft website):
1) Put in "amd64" or "arm64" subdirectory driver files(*.sys and *.inf) to be signed.
2) Copy this folder to the signing machine where the USB token with the certificate is installed.
3) Use sign.bat script, for example:
   sign.bat amd64 windscribesplittunnel (for amd64 architecture)
   sign.bat arm64 windscribesplittunnel (for arm64 architecture)
4) In the subdirectory "disk1" you will find the generated *.cab file.

Step 2 (signing on the Microsoft site):
1) Goto https://partner.microsoft.com/en-us/dashboard/hardware/Search. You must be logged in with your account which has permissions to sign drivers.
2) Click "Submit new hardware".
3) Enter product name (for example, 'Windscribe 2.0 - Split Tunneling arm64').
4) Select and upload your *.cab file generated on previous step.
5) The "Perform test-signing" checkbox should be enabled.
6) Select "Requested Signatures", selecting those related to the architecture of the uploaded driver.
7) Click "Submit" and wait for the driver to be signed by Microsoft.
8) Download signed drivers.