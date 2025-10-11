#include "cat_detect.hpp"
#include "dl_image_jpeg.hpp"
#include "esp_log.h"
#include "bsp/esp-bsp.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

const char *TAG = "cat_detect_sdcard";

// Function to read image file from SD card
uint8_t* read_image_from_sdcard(const char* filepath, size_t* file_size) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate memory for file content
    uint8_t* buffer = (uint8_t*)malloc(*file_size);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for file: %zu bytes", *file_size);
        fclose(file);
        return NULL;
    }
    
    // Read file content
    size_t bytes_read = fread(buffer, 1, *file_size, file);
    fclose(file);
    
    if (bytes_read != *file_size) {
        ESP_LOGE(TAG, "Failed to read complete file. Expected: %zu, Read: %zu", *file_size, bytes_read);
        free(buffer);
        return NULL;
    }
    
    ESP_LOGI(TAG, "Successfully loaded image file: %s (%zu bytes)", filepath, *file_size);
    return buffer;
}

extern "C" void app_main(void)
{
    // Mount SD card
    ESP_LOGI(TAG, "Mounting SD card...");
    esp_err_t ret = bsp_sdcard_mount();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "SD card mounted successfully");

    // Define SD card image path
    const char* image_path = "/sdcard/cat.jpg";
    
    // Read image from SD card
    size_t image_size;
    uint8_t* image_data = read_image_from_sdcard(image_path, &image_size);
    if (!image_data) {
        ESP_LOGE(TAG, "Failed to read image from SD card");
        bsp_sdcard_unmount();
        return;
    }

    // Create JPEG image structure
    dl::image::jpeg_img_t jpeg_img = {
        .data = (void *)image_data, 
        .data_len = image_size
    };
    
    // Decode JPEG image
    ESP_LOGI(TAG, "Decoding JPEG image...");
    auto img = dl::image::sw_decode_jpeg(jpeg_img, dl::image::DL_IMAGE_PIX_TYPE_RGB888);
    if (!img.data) {
        ESP_LOGE(TAG, "Failed to decode JPEG image");
        free(image_data);
        bsp_sdcard_unmount();
        return;
    }
    ESP_LOGI(TAG, "Image decoded successfully: %dx%d", img.width, img.height);

    // Initialize cat detection model
    ESP_LOGI(TAG, "Initializing cat detection model...");
    CatDetect *detect = new CatDetect();
    if (!detect) {
        ESP_LOGE(TAG, "Failed to initialize cat detection model");
        heap_caps_free(img.data);
        free(image_data);
        bsp_sdcard_unmount();
        return;
    }

    // Run cat detection
    ESP_LOGI(TAG, "Running cat detection...");
    auto &detect_results = detect->run(img);
    
    // Display results
    ESP_LOGI(TAG, "Detection results: %d objects found", detect_results.size());
    for (const auto &res : detect_results) {
        ESP_LOGI(TAG,
                 "[category: %d, score: %f, x1: %d, y1: %d, x2: %d, y2: %d]",
                 res.category,
                 res.score,
                 res.box[0],
                 res.box[1],
                 res.box[2],
                 res.box[3]);
    }

    // Cleanup
    delete detect;
    heap_caps_free(img.data);
    free(image_data);
    
    // Unmount SD card
    ESP_LOGI(TAG, "Unmounting SD card...");
    ESP_ERROR_CHECK(bsp_sdcard_unmount());
    ESP_LOGI(TAG, "Cat detection completed successfully");
}