#!/bin/bash

# Switch Cat Detection Example to SD Card Mode
# This script copies the SD card configuration files to replace the embedded mode files

echo "Switching to SD Card Mode for Cat Detection..."

# Backup original files
if [ ! -f "main/app_main_original.cpp" ]; then
    echo "Backing up original files..."
    cp main/app_main.cpp main/app_main_original.cpp
    cp main/CMakeLists.txt main/CMakeLists_original.txt
    if [ -f "sdkconfig" ]; then
        cp sdkconfig sdkconfig.original
    fi
    if [ -f "partitions.csv" ]; then
        cp partitions.csv partitions_original.csv
    fi
fi

# Copy SD card mode files
echo "Copying SD card mode files..."
cp main/app_main_sdcard.cpp main/app_main.cpp
cp main/CMakeLists_sdcard.txt main/CMakeLists.txt
cp sdkconfig.sdcard sdkconfig
cp partitions_sdcard.csv partitions.csv

echo "✅ Successfully switched to SD Card mode!"
echo ""
echo "Cleaning previous build to ensure new configuration is used..."
idf.py clean
echo ""
echo "Next steps:"
echo "1. Prepare your SD card with the directory structure described in README_SDCARD.md"
echo "2. Copy cat.jpg to the root of your SD card"
echo "3. Copy model files to /sdcard/models/s3/ directory"
echo "4. Build and flash: idf.py build flash monitor"
echo ""
echo "To switch back to embedded mode, run: ./switch_to_embedded_mode.sh"