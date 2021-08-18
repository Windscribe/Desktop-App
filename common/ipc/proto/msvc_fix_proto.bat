@echo off
set OUTFILENAME=%1
set MERGEDFILENAME=%OUTFILENAME%.merged

echo #ifdef _MSC_VER>%MERGEDFILENAME%
echo #pragma warning(push)>>%MERGEDFILENAME%
echo #pragma warning(disable: 4018 4100 4267 4244)>>%MERGEDFILENAME%
echo #endif>>%MERGEDFILENAME%

type "%OUTFILENAME%">>%MERGEDFILENAME%

echo #ifdef _MSC_VER>>%MERGEDFILENAME%
echo #pragma warning(pop)>>%MERGEDFILENAME%
echo #endif>>%MERGEDFILENAME%

copy /Y %MERGEDFILENAME% %OUTFILENAME%>NUL
del %MERGEDFILENAME%
