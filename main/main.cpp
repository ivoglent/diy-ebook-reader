//
// Created by long.nguyenviet on 7/18/25.
// Fully fixed integration of LVGL (1bpp) with GDEQ0426T82 (SSD1677) 800x480 ePaper
// - Correct LVGL buffer sizing & flush conversion (1 byte per pixel -> 1 bit per pixel)
// - Proper init order (EPD first, then LVGL)
// - LVGL tick using esp_timer
// - Full-screen only flush (EPD_WhiteScreen_ALL)
// - Optional orientation guard
//

#include <stdio.h>
#include <sstream>

#include <esp_log.h>
#include <esp_check.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_timer.h>
#include <esp_task_wdt.h>
#include <nvs_flash.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <esp_lcd_types.h>
#include "esp_spiffs.h"
#include "components/display/Display.h"

#define TAG "MAIN"
Display* display;

void mount_spiffs() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE("SPIFFS", "Failed to mount SPIFFS (%s)", esp_err_to_name(ret));
        return;
    }

    size_t total = 0, used = 0;
    esp_spiffs_info(NULL, &total, &used);
    ESP_LOGI("SPIFFS", "Partition size: total: %d, used: %d", total, used);
}


static void lvgl_demo_task(void *pv) {
    /*ESP_LOGI(TAG, "Drawing example!");

   // Create a label object
   lv_obj_t *label = lv_label_create(lv_scr_act());

   // Set the text (UTF-8 supported)
   lv_label_set_text(label, "Cộng hòa xã hội chủ nghĩa Việt Nam!");

   // Optionally, set alignment and style
   lv_obj_set_style_text_color(label, lv_color_black(), 0);
   lv_obj_set_style_text_font(label, &roboto_24, 0);  // choose font size
   lv_obj_center(label);

   // Force an immediate refresh once objects created
   delay_ms(500);
   lv_refr_now(NULL);*/
    int i = 1;
    while (i < 10) {
        std::stringstream ss;
        ss << "/spiffs/pages/page_00";
        ss << i;
        ss << ".bmp";
        display->display(ss.str());
        vTaskDelay(pdMS_TO_TICKS(3000));
        i++;
        if (i > 9) i =1;
    }
    vTaskDelete(nullptr);
}


extern "C" void app_main(void) {
    // NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS init done");
    mount_spiffs();
    display = new Display();
    display->setup();
    xTaskCreatePinnedToCore(lvgl_demo_task, "lvgl_demo_task", 1024 * 6, NULL, 3, NULL, 1);
    ESP_LOGI(TAG, "Setup complete");
}
