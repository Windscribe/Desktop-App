@echo off
setlocal
set tools_dir=%~dp0
set PYTHONDONTWRITEBYTECODE=1
python3 "%tools_dir%\ws_translate.py" %*
exit /B %ERRORLEVEL%
