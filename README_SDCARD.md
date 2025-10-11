# Cat Detection with SD Card Mode

This example demonstrates cat detection using ESP32-S3 deep learning capabilities with images and models loaded from SD card instead of embedded binaries.

## Features

- Loads cat images from SD card at runtime
- Loads detection model from SD card
- Full error handling for SD card operations
- Memory efficient - doesn't embed large files in flash
- Supports multiple model types (224x224 and 416x416)

## Hardware Requirements

- ESP32-S3 development board (e.g., ESP32-S3-EYE)
- MicroSD card (formatted as FAT32)
- SD card slot/reader

## SD Card Setup

### Directory Structure
Create the following directory structure on your SD card:

```
/sdcard/
├── cat.jpg                          # Input image for detection
└── models/
    └── s3/                          # For ESP32-S3 target
        ├── espdet_pico_224_224_cat.espdl
        └── espdet_pico_416_416_cat.espdl
```

### Files to Copy
1. **Image file**: Copy your cat image as `cat.jpg` to the root of the SD card
   - Supported formats: JPEG
   - Recommended size: 320x240 to 640x480 pixels

2. **Model files**: Copy the model files from the project:
   - From: `models/cat_detect/models/s3/espdet_pico_224_224_cat.espdl`
   - To: `/sdcard/models/s3/espdet_pico_224_224_cat.espdl`

## Build Configuration

### Using the provided configuration:

**IMPORTANT: You MUST use the switch script or manually copy the configuration files:**

**Option 1: Use the switch script (Recommended)**
```bash
# Windows
switch_to_sdcard_mode.bat

# Linux/Mac
./switch_to_sdcard_mode.sh
```

**Option 2: Manual configuration**
```bash
# Copy the SD card configuration
cp sdkconfig.sdcard sdkconfig

# Copy the modified CMakeLists.txt
cp main/CMakeLists_sdcard.txt main/CMakeLists.txt

# Copy the modified main file
cp main/app_main_sdcard.cpp main/app_main.cpp

# Copy the partition table
cp partitions_sdcard.csv partitions.csv

# Clean and rebuild to ensure new config is used
idf.py clean
```

### Manual configuration via menuconfig:
```bash
idf.py menuconfig
```

Navigate to:
- **models: cat_detect** → **model location** → Select **sdcard**
- **models: cat_detect** → **default model** → Select desired model
- **Component config** → **FAT Filesystem support** → Enable long filename support

## Build and Flash

```bash
# Configure the project for ESP32-S3
idf.py set-target esp32s3

# Build the project
idf.py build

# Flash to device
idf.py flash

# Monitor output
idf.py monitor
```

## Usage

1. Insert the prepared SD card into your ESP32-S3 board
2. Reset the board or power cycle
3. The application will:
   - Mount the SD card
   - Load the cat image from `/sdcard/cat.jpg`
   - Load the detection model from SD card
   - Run cat detection
   - Display results via serial output
   - Unmount the SD card

## Expected Output

```
I (xxx) cat_detect_sdcard: Mounting SD card...
I (xxx) cat_detect_sdcard: SD card mounted successfully
I (xxx) cat_detect_sdcard: Successfully loaded image file: /sdcard/cat.jpg (xxxxx bytes)
I (xxx) cat_detect_sdcard: Decoding JPEG image...
I (xxx) cat_detect_sdcard: Image decoded successfully: 640x480
I (xxx) cat_detect_sdcard: Initializing cat detection model...
I (xxx) cat_detect_sdcard: Running cat detection...
I (xxx) cat_detect_sdcard: Detection results: 1 objects found
I (xxx) cat_detect_sdcard: [category: 0, score: 0.850000, x1: 120, y1: 80, x2: 300, y2: 250]
I (xxx) cat_detect_sdcard: Unmounting SD card...
I (xxx) cat_detect_sdcard: Cat detection completed successfully
```

## Troubleshooting

### "Warning: Long filenames on SD card are disabled" Message
This warning appears when the FATFS long filename support is not properly configured. **IMPORTANT**: This happens when you haven't switched to the SD card configuration yet.

**Solution:**
1. **Run the switch script** (this is the most important step):
   ```bash
   # Windows
   switch_to_sdcard_mode.bat
   
   # Linux/Mac
   ./switch_to_sdcard_mode.sh
   ```

2. **Manual verification** - Check that your `sdkconfig` file contains:
   ```
   CONFIG_FATFS_LFN_STACK=y
   CONFIG_FATFS_MAX_LFN=255
   ```

3. **Force clean rebuild**:
   ```bash
   idf.py clean
   idf.py build
   ```

4. **Verify files are copied correctly**:
   - `sdkconfig` should match `sdkconfig.sdcard`
   - `main/app_main.cpp` should match `main/app_main_sdcard.cpp`
   - `partitions.csv` should match `partitions_sdcard.csv`

### SD Card Mount Issues
- Ensure SD card is formatted as FAT32
- Check SD card connections
- Verify CONFIG_BSP_SD_MOUNT_POINT is set correctly

### File Not Found Errors
- Verify directory structure on SD card
- Check file permissions
- Ensure files are not corrupted

### Memory Issues
- Increase heap size if needed
- Ensure SPIRAM is enabled for large images
- Check available heap before operations

### Model Loading Errors
- Verify correct model file for target (S3 vs P4)
- Check model file integrity
- Ensure correct path configuration

## Configuration Options

Key configuration parameters in `sdkconfig`:

```
CONFIG_CAT_DETECT_MODEL_IN_SDCARD=y
CONFIG_CAT_DETECT_MODEL_LOCATION=2
CONFIG_CAT_DETECT_MODEL_SDCARD_DIR="models/s3"
CONFIG_BSP_SD_MOUNT_POINT="/sdcard"
```

## Performance Notes

- SD card access is slower than flash/RAM access
- First run may take longer due to model loading
- SPIRAM usage is recommended for large images
- Consider caching frequently used models in RAM if memory permits

## Comparison with Embedded Mode

| Feature | Embedded Mode | SD Card Mode |
|---------|---------------|--------------|
| Flash usage | High (models + images) | Low (code only) |
| Boot time | Fast | Slower (SD card init) |
| Flexibility | Fixed content | Runtime changeable |
| Development | Rebuild for changes | Just replace files |
| Storage capacity | Limited by flash | Limited by SD card size |
