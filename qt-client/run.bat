@echo off
REM Run the ESP32-CAM Qt desktop client. Builds first if needed.
REM Double-click this file, or run from PowerShell/CMD: .\run.bat
setlocal

if "%QT_DIR%"=="" set "QT_DIR=C:\Qt\6.11.1\mingw_64"
if "%MINGW_DIR%"=="" set "MINGW_DIR=C:\Qt\Tools\mingw1310_64"

cd /d "%~dp0"

REM Ensure Qt and MinGW runtime DLLs are findable at launch.
set "PATH=%MINGW_DIR%\bin;%QT_DIR%\bin;%PATH%"

if not exist "build\esp32cam_client.exe" (
  echo Executable not found; building first...
  call "%~dp0build.bat"
  if errorlevel 1 exit /b 1
)

start "" "build\esp32cam_client.exe"
endlocal
