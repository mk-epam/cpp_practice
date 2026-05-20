@echo off
if "%~3"=="" (
    echo Usage: copy_ipc.bat ^<source^> ^<target^> ^<shared_memory_name^>
    exit /b 4
)
start "" /B copytool.exe %1 %2 %3
timeout /t 1 /nobreak >nul
copytool.exe %1 %2 %3
exit /b %ERRORLEVEL%