ECHO off
ECHO ===== Installing Visual Studio Community 2019 =====
call vs_Community.exe --add Microsoft.VisualStudio.Workload.NativeDesktop --quiet --wait --includeRecommended --norestart
IF ERRORLEVEL 0 (
   echo Finished Successfully, error code: %errorlevel%
) ELSE (
   echo Finished with Failure, error code: %errorlevel%
)
