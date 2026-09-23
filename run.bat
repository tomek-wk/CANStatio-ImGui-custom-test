@echo off
setlocal
cd /d "%~dp0"

if not exist "build\canstatio_imgui_custom_test.exe" (
    echo Executable not found. Run update.bat first.
    pause
    exit /b 1
)

build\canstatio_imgui_custom_test.exe
exit /b %errorlevel%
