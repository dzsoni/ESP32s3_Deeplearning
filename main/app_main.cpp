#include "cat_detect.hpp"
#include "dl_image_jpeg.hpp"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

extern const uint8_t cat_jpg_start[] asm("_binary_cat_jpg_start");
extern const uint8_t cat_jpg_end[] asm("_binary_cat_jpg_end");
const char *TAG = "cat_detect_partition";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting Cat Detection with Flash Partition Mode");

#if CONFIG_CAT_DETECT_MODEL_IN_SDCARD
    ESP_ERROR_CHECK(bsp_sdcard_mount());
#endif

    // Decode the embedded JPEG image
    dl::image::jpeg_img_t jpeg_img = {.data = (void *)cat_jpg_start, .data_len = (size_t)(cat_jpg_end - cat_jpg_start)};
    auto img = dl::image::sw_decode_jpeg(jpeg_img, dl::image::DL_IMAGE_PIX_TYPE_RGB888);
    ESP_LOGI(TAG, "Image decoded successfully - Width: %d, Height: %d", img.width, img.height);

    // Initialize Cat Detection model - configured for partition mode in menuconfig
    CatDetect *detect = new CatDetect();
    ESP_LOGI(TAG, "Cat detection model initialized.");

    // Run inference
    auto &detect_results = detect->run(img);
    ESP_LOGI(TAG, "Detection completed, found %d results", detect_results.size());
    
    // Display results
    for (const auto &res : detect_results) {
        ESP_LOGI(TAG,
                 "Detection [category: %d, score: %.3f, bbox: x1=%d, y1=%d, x2=%d, y2=%d]",
                 res.category,
                 res.score,
                 res.box[0],
                 res.box[1],
                 res.box[2],
                 res.box[3]);
    }

    // Clean up
    delete detect;
    heap_caps_free(img.data);
    ESP_LOGI(TAG, "Cat detection completed successfully");

#if CONFIG_CAT_DETECT_MODEL_IN_SDCARD
    ESP_ERROR_CHECK(bsp_sdcard_unmount());
#endif
}
