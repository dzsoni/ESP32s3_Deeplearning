@echo off
REM Switch Cat Detection Example to SD Card Mode
REM This script copies the SD card configuration files to replace the embedded mode files

echo Switching to SD Card Mode for Cat Detection...

REM Backup original files
if not exist "main\app_main_original.cpp" (
    echo Backing up original files...
    copy "main\app_main.cpp" "main\app_main_original.cpp" >nul
    copy "main\CMakeLists.txt" "main\CMakeLists_original.txt" >nul
    if exist "sdkconfig" copy "sdkconfig" "sdkconfig.original" >nul
    if exist "partitions.csv" copy "partitions.csv" "partitions_original.csv" >nul
)

REM Copy SD card mode files
echo Copying SD card mode files...
copy "main\app_main_sdcard.cpp" "main\app_main.cpp" >nul
copy "main\CMakeLists_sdcard.txt" "main\CMakeLists.txt" >nul
copy "sdkconfig.sdcard" "sdkconfig" >nul
copy "partitions_sdcard.csv" "partitions.csv" >nul

echo.
echo ✅ Successfully switched to SD Card mode!
echo.
echo Cleaning previous build to ensure new configuration is used...
idf.py clean
echo.
echo Next steps:
echo 1. Prepare your SD card with the directory structure described in README_SDCARD.md
echo 2. Copy cat.jpg to the root of your SD card
echo 3. Copy model files to /sdcard/models/s3/ directory
echo 4. Build and flash: idf.py build flash monitor
echo.
echo To switch back to embedded mode, run: switch_to_embedded_mode.bat
pause