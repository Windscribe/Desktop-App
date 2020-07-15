ECHO OFF
@RD /S /Q %~dp0..\generated_proto 
mkdir %~dp0..\generated_proto

forfiles /p %~dp0 /m *.proto /c "c:/libs/protobuf_release/bin/protoc -I=%~dp0 --cpp_out=%~dp0..\generated_proto @file"