@echo off
set "TOOL=%~dp0..\build\copytool.exe"
if "%~3"=="" (
    echo Usage: copy_ipc.bat ^<source^> ^<target^> ^<shared_memory_name^>
    exit /b 4
)
start "" /B "%TOOL%" %1 %2 %3
timeout /t 1 /nobreak >nul
"%TOOL%" %1 %2 %3
exit /b %ERRORLEVEL%
