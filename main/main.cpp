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
#include <driver/spi_master.h>
#include <esp_timer.h>
#include <nvs_flash.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_spiffs.h"
#include "components/controller/Button.h"
#include "components/controller/Menu.h"
#include "components/display/Display.h"
#include "components/display/screen/ScreenManager.h"
#include "components/sdcard/SDCard.h"
#include "components/system/Registry.h"
#include "components/system/EventBus.h"
#include "components/system/Global.h"

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


template <typename T, typename ...Args>
void register_service(Args&&... all) {
    getRegistryInstance().create<T>(std::forward<Args>(all)...);
}

void setup() {
    for (const auto service: getRegistryInstance().getServices()) {
        service->setup();
    }

    for (const auto service: getRegistryInstance().getServices()) {
        service->subscribe();
    }
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

    event_bus_init();
    ESP_LOGI(TAG, "Init event bus done");

    register_service<SDCard>(static_cast<gpio_num_t>(CONFIG_SDCARD_SPI_MOSI), static_cast<gpio_num_t>(CONFIG_SDCARD_SPI_MISO), static_cast<gpio_num_t>(CONFIG_SDCARD_SPI_CLK), static_cast<gpio_num_t>(CONFIG_SDCARD_SPI_CS));
    vTaskDelay(pdMS_TO_TICKS(1000));
    mount_spiffs();
    register_service<Display>();
    register_service<Button>(CONFIG_BUTTON_PIN_UP, CONFIG_BUTTON_PIN_DOWN, CONFIG_BUTTON_PIN_SELECT);

    register_service<ScreenManager>();
    setup();
    ESP_LOGI(TAG, "Setup complete");
}
