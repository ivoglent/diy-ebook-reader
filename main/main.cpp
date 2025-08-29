//
// Created by long.nguyenviet on 7/18/25.
// Fully fixed integration of LVGL (1bpp) with GDEQ0426T82 (SSD1677) 800x480 ePaper
// - Correct LVGL buffer sizing & flush conversion (1 byte per pixel -> 1 bit per pixel)
// - Proper init order (EPD first, then LVGL)
// - LVGL tick using esp_timer
// - Full-screen only flush (EPD_WhiteScreen_ALL)
// - Optional orientation guard
//

#include <esp_console.h>
#include <stdio.h>
#include <sstream>

#include <esp_log.h>
#include <driver/spi_master.h>
#include <esp_timer.h>
#include <nvs_flash.h>
#include <linenoise/linenoise.h>

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
#include "console.h"

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

#define ITEM_COUNT 9
#define COLS 3

void create_grid_screen() {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_size(scr, 480, 800);
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

    // Grid layout: 3 cột × 3 hàng
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(scr, col_dsc, row_dsc);

    for (int i = 0; i < 9; i++) {
        lv_obj_t *item = lv_obj_create(scr);

        // item sẽ tự động chiếm hết cell
        lv_obj_set_grid_cell(item,
                             LV_GRID_ALIGN_STRETCH, i % 3, 1,
                             LV_GRID_ALIGN_STRETCH, i / 3, 1);

        // Style border mỏng
        static lv_style_t style_border;
        lv_style_init(&style_border);
        lv_style_set_border_color(&style_border, lv_color_black());
        lv_style_set_border_width(&style_border, 1);
        lv_obj_add_style(item, &style_border, 0);

        if (i == 0) {
            // Style cho item đầu (selected)
            static lv_style_t style_selected;
            lv_style_init(&style_selected);
            lv_style_set_border_color(&style_selected, lv_color_black());
            lv_style_set_border_width(&style_selected, 3);
            lv_obj_add_style(item, &style_selected, 0);
        }

        // Label
        lv_obj_t *label = lv_label_create(item);
        lv_label_set_text_fmt(label, "Item %d", i + 1);
        lv_obj_center(label);
    }

    lv_scr_load(scr);
    vTaskDelay(pdMS_TO_TICKS(100));
    lv_refr_now(nullptr);
}

void create_grid_screen2() {
    // Tạo screen mới
    lv_obj_t *scr = lv_obj_create(NULL);

    // Cấu hình layout grid
    static lv_coord_t col_dsc[] = { 150, 150, 150, LV_GRID_TEMPLATE_LAST }; // 3 cột
    static lv_coord_t row_dsc[] = { 80, 80, LV_GRID_TEMPLATE_LAST };        // 2 hàng

    lv_obj_set_grid_dsc_array(scr, col_dsc, row_dsc);
    lv_obj_set_layout(scr, LV_LAYOUT_GRID);

    // Tạo 6 nút theo grid
    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 3; c++) {
            lv_obj_t *btn = lv_btn_create(scr);
            lv_obj_set_grid_cell(btn,
                                 LV_GRID_ALIGN_STRETCH, c, 1,  // cột
                                 LV_GRID_ALIGN_STRETCH, r, 1); // hàng

            lv_obj_t *label = lv_label_create(btn);
            lv_label_set_text_fmt(label, "Btn %d", r * 3 + c + 1);
            lv_obj_center(label);
        }
    }

    // Load màn hình ra
    lv_scr_load(scr);
    vTaskDelay(pdMS_TO_TICKS(100));
    lv_refr_now(nullptr);
}

void test(void* p)
{
    ESP_LOGI("MAIN", "Starting test");
    /*std::vector<std::string> titles;
    titles.emplace_back("Testing title 1");
    titles.emplace_back("Testing title 2");
    titles.emplace_back("Testing title 3");
    titles.emplace_back("Testing title 4");
    titles.emplace_back("Testing title 5");
    titles.emplace_back("Testing title 6");
    titles.emplace_back("Testing title 7");
    titles.emplace_back("Testing title 8");
    lv_obj_t* book_items[8];
    lv_obj_t* book_labels[8];
    int count = titles.size();
    auto _data = lv_obj_create(lv_scr_act());
    lv_obj_set_style_bg_color(_data, lv_color_white(), 0);
    int page_items = (count < ITEMS_PER_PAGE) ? count : ITEMS_PER_PAGE;
    if (page_items > titles.size()) page_items = titles.size();

    for (int i = 0; i < page_items; i++) {
        int col = i % PAGE_COLS;
        int row = i / PAGE_COLS;

        int x = ITEM_MARGIN + col * (ITEM_W + ITEM_MARGIN);
        int y = ITEM_MARGIN + row * (ITEM_H + ITEM_MARGIN);

        // Box
        book_items[i] = lv_obj_create(_data);
        lv_obj_set_size(book_items[i], ITEM_W, ITEM_H);
        lv_obj_set_pos(book_items[i], x, y);
        lv_obj_set_style_border_width(book_items[i], 1, 0);
        lv_obj_set_style_border_color(book_items[i], lv_color_black(), 0);
        lv_obj_set_style_bg_color(book_items[i], lv_color_white(), 0);
        lv_obj_set_style_radius(book_items[i], 0, 0);

        // Title
        book_labels[i] = lv_label_create(book_items[i]);
        lv_label_set_text(book_labels[i], titles[i].c_str());
        lv_obj_center(book_labels[i]);
    }
    lv_scr_load(_data);
    vTaskDelay(pdMS_TO_TICKS(1000));
    lv_refr_now(nullptr);*/
    //vTaskDelay(pdMS_TO_TICKS(1000));
    /*ESP_LOGI("MAIN", "Starting test");
    lv_obj_t *rect = lv_obj_create(lv_scr_act());
    lv_obj_set_size(rect, 200, 200);
    lv_obj_center(rect);
    lv_obj_set_style_radius(rect, 0, 0); // sharp corners
    lv_obj_set_style_bg_color(rect, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);

    // Delay a bit and then force flush
    vTaskDelay(pdMS_TO_TICKS(100));
    lv_refr_now(nullptr);*/
    create_grid_screen();
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void console_setup()
{
    esp_console_repl_t *repl = nullptr;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();

    repl_config.prompt = CONFIG_IDF_TARGET
                         ">";
    repl_config.max_cmdline_length = 4096;
    repl_config.task_stack_size = 8912;
    repl_config.history_save_path = "/storage/history.txt";
    esp_console_register_help_command();
    ESP_LOGI("MAIN", "Registering commands: %d", COMMANDS.size());
    for (const auto &cmd: COMMANDS) {
        ESP_LOGI("MAIN", "Registered command: %s", cmd.command);
        ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));
    }

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
    esp_console_dev_uart_config_t hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
    esp_console_dev_usb_cdc_config_t hw_config = ESP_CONSOLE_DEV_CDC_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_cdc(&hw_config, &repl_config, &repl));

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
#else
#error Unsupported console type
#endif

    ESP_ERROR_CHECK(esp_console_start_repl(repl));
    ESP_LOGI("MAIN", "console started");
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

    //register_service<SDCard>(static_cast<gpio_num_t>(CONFIG_SDCARD_SPI_MOSI), static_cast<gpio_num_t>(CONFIG_SDCARD_SPI_MISO), static_cast<gpio_num_t>(CONFIG_SDCARD_SPI_CLK), static_cast<gpio_num_t>(CONFIG_SDCARD_SPI_CS));
    //vTaskDelay(pdMS_TO_TICKS(1000));
    mount_spiffs();
    register_service<Display>();
    register_service<Button>(CONFIG_BUTTON_PIN_UP, CONFIG_BUTTON_PIN_DOWN, CONFIG_BUTTON_PIN_SELECT);

    register_service<ScreenManager>();
    setup();
    ESP_LOGI(TAG, "Setup complete");
    //xTaskCreatePinnedToCore(test, "test-task", 4096, NULL, 5, NULL, 1);

    console_setup();
}
