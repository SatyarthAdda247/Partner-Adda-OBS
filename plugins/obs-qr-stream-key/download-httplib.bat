@echo off
REM Download cpp-httplib header for Windows

echo Downloading cpp-httplib...
powershell -Command "Invoke-WebRequest -Uri 'https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h' -OutFile 'httplib.h'"

if %ERRORLEVEL% EQU 0 (
    echo Successfully downloaded httplib.h
) else (
    echo Failed to download httplib.h
    exit /b 1
)
