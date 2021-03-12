ECHO OFF
@RD /S /Q %~dp0..\generated_proto 
mkdir %~dp0..\generated_proto

set oldpath=%PATH%
set PATH=c:/libs/protobuf_release/bin;%PATH%
forfiles /p %~dp0 /m *.proto /c "protoc -I=%~dp0 --cpp_out=%~dp0..\generated_proto @file"
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
  del merged.tmp
  echo %headstring1:"=% >> merged.tmp
  echo %headstring2:"=% >> merged.tmp
  echo %headstring3:"=% >> merged.tmp
  echo %headstring4:"=% >> merged.tmp

  type "%%I" >> merged.tmp

  echo %endstring1:"=% >> merged.tmp
  echo %endstring2:"=% >> merged.tmp
  echo %endstring3:"=% >> merged.tmp
  
  copy /Y merged.tmp %%~fI
)

del merged.tmp

REM verify all files were created
for /R %~dp0 %%f in (*.proto) do (
  IF NOT EXIST %~dp0..\generated_proto\%%~nf.pb.cc (goto :error)
  IF NOT EXIST %~dp0..\generated_proto\%%~nf.pb.h (goto :error)
)

echo Successfully generated all ipc files.
goto :eof

:error
  echo Failed to generate all ipc files.
  exit /b 1
  