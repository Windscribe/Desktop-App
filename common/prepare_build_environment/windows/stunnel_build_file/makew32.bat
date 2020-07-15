@echo off
TITLE W32 STUNNEL 

set NEWTGTCPU=W32

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86

nmake.exe -f vc.mak %1 %2 %3 %4 %5 %6 %7 %8 %9
