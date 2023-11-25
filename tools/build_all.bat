@echo off
::------------------------------------------------------------------------------
:: Windscribe Build System
:: Copyright (c) 2020-2023, Windscribe Limited. All rights reserved.
::------------------------------------------------------------------------------
setlocal
set tools_dir=%~dp0
set PYTHONDONTWRITEBYTECODE=1
set PYTHONIOENCODING=utf_8
chcp 65001

set python_dir=%PYTHONHOME%
if not "%python_dir%" == "" set python_dir=%python_dir%\

%python_dir%python3 "%tools_dir%\build_all.py" %*
exit /B %ERRORLEVEL%
