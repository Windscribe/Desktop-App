@ECHO OFF
@RD /S /Q %~dp0..\generated_proto 
mkdir %~dp0..\generated_proto

set protodir=%1
if "%protodir%"=="" set protodir=%~dp0..\..\..\build-libs\protobuf\release\bin

set oldpath=%PATH%
set PATH=%protodir%;%PATH%
forfiles /p %~dp0 /m *.proto /c "protoc -I=%~dp0 --cpp_out=%~dp0..\generated_proto @file" 1>NUL
set PATH=%oldpath%

rem removing warnings for MSVC
SET headstring1="#ifdef _MSC_VER"
SET headstring2="#pragma warning^(push^)"
SET headstring3="#pragma warning^(disable: 4018 4100 4267 4244^)"
SET headstring4="#endif"

SET endstring1="#ifdef _MSC_VER"
SET endstring2="#pragma warning^(pop^)"
SET endstring3="#endif"

for %%I in (%~dp0..\generated_proto\*.cc) do (
  if exist merged.tmp del merged.tmp
  echo %headstring1:"=% >> merged.tmp
  echo %headstring2:"=% >> merged.tmp
  echo %headstring3:"=% >> merged.tmp
  echo %headstring4:"=% >> merged.tmp

  type "%%I" >> merged.tmp

  echo %endstring1:"=% >> merged.tmp
  echo %endstring2:"=% >> merged.tmp
  echo %endstring3:"=% >> merged.tmp
  
  copy /Y merged.tmp %%~fI 1>NUL
)

if exist merged.tmp del merged.tmp
exit /B %ERRORLEVEL%
