#include "cat_detect.hpp"
#include "dl_image_jpeg.hpp"
#include "esp_log.h"
#include "esp_camera.h"
#include "bsp/esp-bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const char *TAG = "camera_stream_detect";

// Configuration for streaming
#define STREAM_FRAME_RATE_MS    200     // ~5 FPS (200ms between frames)
#define DETECTION_THRESHOLD     0.5f    // Minimum confidence for detections
#define MAX_DETECTIONS_LOG      5       // Maximum detections to log per frame

// Global variables for camera streaming
static bool camera_streaming = true;
static CatDetect *detector = nullptr;

// Camera stream processing task
void camera_stream_task(void *param)
{
    uint32_t frame_count = 0;
    uint32_t detection_count = 0;
    
    ESP_LOGI(TAG, "Starting continuous camera stream processing...");
    
    while (camera_streaming) {
        // Capture frame from camera
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGW(TAG, "Frame %lu: Camera capture failed", frame_count);
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        frame_count++;
        
        // Process every frame but log less frequently
        if (frame_count % 25 == 0) {  // Log every 25th frame (~every 5 seconds at 5 FPS)
            ESP_LOGI(TAG, "Frame %lu: Size: %zu bytes, Format: %d, Dimensions: %dx%d",
                     frame_count, fb->len, fb->format, fb->width, fb->height);
        }

        dl::image::img_t img;
        bool img_decoded = false;
        bool needs_free = false;  // Track if we need to free img.data
        
        if (fb->format == PIXFORMAT_JPEG) {
            dl::image::jpeg_img_t jpeg_img = {
                .data = (void *)fb->buf,
                .data_len = fb->len
            };
            img = dl::image::sw_decode_jpeg(jpeg_img, dl::image::DL_IMAGE_PIX_TYPE_RGB565);
            img_decoded = true;
            needs_free = true;  // JPEG decoder allocates memory that needs to be freed
        } else if (fb->format == PIXFORMAT_RGB565) {
            img = {
                .data = (void *) fb->buf,  // Points directly to camera buffer
                .width = (uint16_t)fb->width,
                .height = (uint16_t)fb->height,
                .pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB565
            };
            img_decoded = true;
            needs_free = false;  // Camera buffer managed by esp_camera_fb_return()
        } else {
            ESP_LOGW(TAG, "Frame %lu: Unsupported pixel format: %d", frame_count, fb->format);
        }

        // Run AI detection if image was successfully decoded
        if (img_decoded && img.data != nullptr && detector != nullptr) {
            auto &detect_results = detector->run(img);
            
            // Filter results by confidence threshold
            uint32_t valid_detections = 0;
            for (const auto &res : detect_results) {
                if (res.score >= DETECTION_THRESHOLD) {
                    valid_detections++;
                    detection_count++;
                    
                    // Log only high-confidence detections and limit logging
                    if (valid_detections <= MAX_DETECTIONS_LOG) {
                        ESP_LOGI(TAG, "Frame %lu Detection #%lu: [cat: %.2f, bbox: %d,%d,%d,%d]",
                                 frame_count, valid_detections, res.score,
                                 res.box[0], res.box[1], res.box[2], res.box[3]);
                    }
                }
            }
            
            if (valid_detections > MAX_DETECTIONS_LOG) {
                ESP_LOGI(TAG, "Frame %lu: ... and %lu more detections",
                         frame_count, valid_detections - MAX_DETECTIONS_LOG);
            }
            
            // Only free memory if it was allocated by JPEG decoder
            if (needs_free && img.data != nullptr) {
                heap_caps_free(img.data);
            }
        }
        
        // Return camera frame buffer
        esp_camera_fb_return(fb);
        
        // Log statistics periodically
        if (frame_count % 50 == 0) {  // Every 50 frames (~10 seconds)
            ESP_LOGI(TAG, "Stream Stats: %lu frames processed, %lu detections found",
                     frame_count, detection_count);
        }
        
        // Control frame rate
        vTaskDelay(pdMS_TO_TICKS(STREAM_FRAME_RATE_MS));
    }
    
    ESP_LOGI(TAG, "Camera stream task ended. Total: %lu frames, %lu detections",
             frame_count, detection_count);
    vTaskDelete(NULL);
}

extern "C" void app_main(void)
{
#if CONFIG_CAT_DETECT_MODEL_IN_SDCARD
    ESP_ERROR_CHECK(bsp_sdcard_mount());
#endif

    // Manual camera configuration for ESP32-S3-EYE
    // Using explicit pin assignments instead of BSP defaults
    camera_config_t camera_config;
    
    // LEDC configuration
    camera_config.ledc_channel = LEDC_CHANNEL_0;
    camera_config.ledc_timer = LEDC_TIMER_0;
    
    // Data pins (D0-D7)
    camera_config.pin_d0 = BSP_CAMERA_D0;      // GPIO_NUM_11
    camera_config.pin_d1 = BSP_CAMERA_D1;      // GPIO_NUM_9
    camera_config.pin_d2 = BSP_CAMERA_D2;      // GPIO_NUM_8
    camera_config.pin_d3 = BSP_CAMERA_D3;      // GPIO_NUM_10
    camera_config.pin_d4 = BSP_CAMERA_D4;      // GPIO_NUM_12
    camera_config.pin_d5 = BSP_CAMERA_D5;      // GPIO_NUM_18
    camera_config.pin_d6 = BSP_CAMERA_D6;      // GPIO_NUM_17
    camera_config.pin_d7 = BSP_CAMERA_D7;      // GPIO_NUM_16
    
    // Clock and sync pins
    camera_config.pin_xclk = BSP_CAMERA_XCLK;  // GPIO_NUM_15
    camera_config.pin_pclk = BSP_CAMERA_PCLK;  // GPIO_NUM_13
    camera_config.pin_vsync = BSP_CAMERA_VSYNC; // GPIO_NUM_6
    camera_config.pin_href = BSP_CAMERA_HSYNC;  // GPIO_NUM_7
    
    // I2C/SCCB pins for sensor communication
    camera_config.pin_sccb_sda = BSP_I2C_SDA;  // GPIO_NUM_4
    camera_config.pin_sccb_scl = BSP_I2C_SCL;  // GPIO_NUM_5
    
    // Power and reset pins
    camera_config.pin_pwdn = GPIO_NUM_NC;       // Not connected
    camera_config.pin_reset = GPIO_NUM_NC;      // Not connected
    
    // Clock and frame configuration
    camera_config.xclk_freq_hz = 10000000;     // 10MHz
    camera_config.frame_size = FRAMESIZE_VGA;   // 640x480
    camera_config.pixel_format = PIXFORMAT_RGB565; // RGB565 for direct processing
    camera_config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    camera_config.fb_location = CAMERA_FB_IN_PSRAM;
    camera_config.jpeg_quality = 12;           // Not used with RGB565
    camera_config.fb_count = 2;                // Double buffering
    camera_config.sccb_i2c_port = -1;          // Use software I2C
    
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return;
    }
    
    // Configure camera settings
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_vflip(s, BSP_CAMERA_VFLIP);        // Vertical flip
        s->set_hmirror(s, BSP_CAMERA_HMIRROR);    // Horizontal mirror
        s->set_brightness(s, 1);                  // Slightly brighter
        s->set_contrast(s, 1);                    // Slightly more contrast
        s->set_saturation(s, 0);                  // Default saturation
    }

    ESP_LOGI(TAG, "Camera initialized for streaming (Format: RGB565, Size: 640x480 VGA)");

    // Initialize AI detector
    detector = new CatDetect();
    if (!detector) {
        ESP_LOGE(TAG, "Failed to initialize cat detector");
        return;
    }
    ESP_LOGI(TAG, "AI detector initialized successfully");

    // Test single frame capture first
    camera_fb_t *test_fb = esp_camera_fb_get();
    if (test_fb) {
        ESP_LOGI(TAG, "Test capture successful: %zu bytes", test_fb->len);
        esp_camera_fb_return(test_fb);
    } else {
        ESP_LOGE(TAG, "Test capture failed - check camera connection");
        delete detector;
        return;
    }

    // Start camera streaming task
    ESP_LOGI(TAG, "Starting camera stream processing task...");
    ESP_LOGI(TAG, "Stream Settings: %d ms/frame (~%.1f FPS), Detection threshold: %.2f",
             STREAM_FRAME_RATE_MS, 1000.0f / STREAM_FRAME_RATE_MS, DETECTION_THRESHOLD);
    
    xTaskCreate(camera_stream_task, "camera_stream", 16384, NULL, 5, NULL);
    
    // Main task can do other work or just wait
    ESP_LOGI(TAG, "Camera streaming started. Press RESET to stop.");
    
    // Keep main task alive
    while (camera_streaming) {
        vTaskDelay(pdMS_TO_TICKS(5000));  // Check every 5 seconds
    }
    
    // Cleanup (this part won't be reached unless streaming is stopped)
    delete detector;
    detector = nullptr;
    
#if CONFIG_CAT_DETECT_MODEL_IN_SDCARD
    ESP_ERROR_CHECK(bsp_sdcard_unmount());
#endif
}
