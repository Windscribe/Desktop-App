@echo off
::------------------------------------------------------------------------------
:: Windscribe Build System
:: Copyright (c) 2020-2021, Windscribe Limited. All rights reserved.
::------------------------------------------------------------------------------
setlocal
set tools_dir=%~dp0
set pause_on_exit=0
echo %cmdcmdline% | find /i "%~0" >nul
if not errorlevel 1 set pause_on_exit=1
set PYTHONDONTWRITEBYTECODE=1

python "%tools_dir%\install_hooks.py"
if _%pause_on_exit%_==_1_ pause
