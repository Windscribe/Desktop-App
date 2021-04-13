@echo off
::------------------------------------------------------------------------------
:: Windscribe Build System
:: Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
::------------------------------------------------------------------------------
setlocal
set tools_dir=%~dp0
set pause_on_exit=0
if not _%RUNNING_CI%_==_1_ (
  echo %cmdcmdline% | find /i "%~0" >nul
  if not errorlevel 1 set pause_on_exit=1
)
set PYTHONDONTWRITEBYTECODE=1

set python_dir=%PYTHONHOME%
if not "%python_dir%" == "" set python_dir=%python_dir%\

%python_dir%python "%tools_dir%\install_openssl.py" %*
if _%pause_on_exit%_==_1_ pause
exit /B %ERRORLEVEL%
