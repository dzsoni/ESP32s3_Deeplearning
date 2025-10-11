#!/bin/bash

# Switch Cat Detection Example back to Embedded Mode
# This script restores the original embedded mode files

echo "Switching back to Embedded Mode for Cat Detection..."

# Check if backup files exist
if [ ! -f "main/app_main_original.cpp" ]; then
    echo "❌ No backup files found. Cannot switch back to embedded mode."
    echo "Please restore from git or manually configure embedded mode."
    exit 1
fi

# Restore original files
echo "Restoring original files..."
cp main/app_main_original.cpp main/app_main.cpp
cp main/CMakeLists_original.txt main/CMakeLists.txt

if [ -f "sdkconfig.original" ]; then
    cp sdkconfig.original sdkconfig
fi

if [ -f "partitions_original.csv" ]; then
    cp partitions_original.csv partitions.csv
fi

echo "✅ Successfully switched back to Embedded mode!"
echo ""
echo "Next steps:"
echo "1. Build and flash: idf.py build flash monitor"
echo ""
echo "To switch back to SD card mode, run: ./switch_to_sdcard_mode.sh"