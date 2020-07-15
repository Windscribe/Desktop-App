ECHO off
ECHO ===== Installing Visual Studio Community 2017 =====
call vs_community__629168339.1549471159.exe --add Microsoft.VisualStudio.Workload.NativeDesktop --quiet --wait --includeRecommended --norestart
IF ERRORLEVEL 0 (
   echo Finished Successfully, error code: %errorlevel%
) ELSE (
   echo Finished with Failure, error code: %errorlevel%
)
