@echo off
REM Build the ESP32-CAM Qt desktop client (Windows + MinGW/Qt).
REM Double-click this file, or run from PowerShell/CMD: .\build.bat
setlocal

if "%QT_DIR%"=="" set "QT_DIR=C:\Qt\6.11.1\mingw_64"
if "%MINGW_DIR%"=="" set "MINGW_DIR=C:\Qt\Tools\mingw1310_64"
if "%NINJA%"=="" set "NINJA=C:\Qt\Tools\Ninja\ninja.exe"

cd /d "%~dp0"

REM MinGW bin must be on PATH so the compiler finds its runtime DLLs.
set "PATH=%MINGW_DIR%\bin;%QT_DIR%\bin;%PATH%"

REM Clean stale cache unless called with --keep
if /I not "%~1"=="--keep" if exist build rmdir /s /q build

echo Configuring (Qt: %QT_DIR%)...
cmake -B build -S . -G Ninja ^
  -DCMAKE_MAKE_PROGRAM="%NINJA%" ^
  -DCMAKE_PREFIX_PATH="%QT_DIR%" ^
  -DCMAKE_C_COMPILER="%MINGW_DIR%/bin/gcc.exe" ^
  -DCMAKE_CXX_COMPILER="%MINGW_DIR%/bin/g++.exe"
if errorlevel 1 goto :fail

echo Building...
cmake --build build
if errorlevel 1 goto :fail

REM Bundle Qt DLLs so the exe runs standalone (ignore failures)
"%QT_DIR%\bin\windeployqt.exe" --release build\esp32cam_client.exe >nul 2>&1

echo.
echo Done -^> build\esp32cam_client.exe
endlocal
exit /b 0

:fail
echo.
echo BUILD FAILED.
endlocal
exit /b 1
