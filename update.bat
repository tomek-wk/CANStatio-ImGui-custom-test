@echo off
setlocal
cd /d "%~dp0"

echo === Git pull ===
git pull
if errorlevel 1 goto :error

echo.
echo === CMake configure ===
cmake -S . -B build -G Ninja
if errorlevel 1 goto :error

echo.
echo === Build ===
cmake --build build
if errorlevel 1 goto :error

echo.
echo === Run ===
build\canstatio_imgui_custom_test.exe
exit /b %errorlevel%

:error
echo.
echo Update/build failed.
pause
exit /b 1
