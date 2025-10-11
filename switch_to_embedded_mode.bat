@echo off
REM Switch Cat Detection Example back to Embedded Mode
REM This script restores the original embedded mode files

echo Switching back to Embedded Mode for Cat Detection...

REM Check if backup files exist
if not exist "main\app_main_original.cpp" (
    echo ❌ No backup files found. Cannot switch back to embedded mode.
    echo Please restore from git or manually configure embedded mode.
    pause
    exit /b 1
)

REM Restore original files
echo Restoring original files...
copy "main\app_main_original.cpp" "main\app_main.cpp" >nul
copy "main\CMakeLists_original.txt" "main\CMakeLists.txt" >nul

if exist "sdkconfig.original" copy "sdkconfig.original" "sdkconfig" >nul
if exist "partitions_original.csv" copy "partitions_original.csv" "partitions.csv" >nul

echo.
echo ✅ Successfully switched back to Embedded mode!
echo.
echo Next steps:
echo 1. Build and flash: idf.py build flash monitor
echo.
echo To switch back to SD card mode, run: switch_to_sdcard_mode.bat
pause