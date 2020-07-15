@echo off
set SCRIPT_PATH=%~dp0
rem echo %SCRIPT_PATH%
set IPC_PROTO_GEN_SCRIPT=%SCRIPT_PATH%IPC\proto\generate_proto.bat

IF EXIST %IPC_PROTO_GEN_SCRIPT% (
	call %IPC_PROTO_GEN_SCRIPT%
) ELSE (
	echo Couldn't find proto script
	PAUSE
)
