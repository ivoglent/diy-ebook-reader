//
// Created by ivoglent on 8/24/2025.
//

#include "Button.h"

#include "esp_log.h"

static const char* BUTTON_TAG = "Button";
button_handle_t btnUp = nullptr;
button_handle_t btnDown = nullptr;
button_handle_t btnSelect = nullptr;
static void button_single_click_cb(void *arg,void *usr_data)
{
    ESP_LOGI(BUTTON_TAG, "BUTTON_SINGLE_CLICK");
}

Button::Button(uint8_t upPin, uint8_t downPin, uint8_t selectPin): _upPin(static_cast<gpio_num_t>(upPin)), _downPin(static_cast<gpio_num_t>(downPin)), _selectPin(static_cast<gpio_num_t>(selectPin)) {
}

void Button::setup() const {
    esp_err_t ret;
    constexpr button_config_t btnUpCfg = {0};
    const button_gpio_config_t btnUpGpioCfg = {
        .gpio_num = _upPin,
        .active_level = 0,
    };
    if(ret = iot_button_new_gpio_device(&btnUpCfg, &btnUpGpioCfg, &btnUp); ret != ESP_OK) {
        ESP_LOGE(BUTTON_TAG, "Button UP create failed");
    }
    iot_button_register_cb(btnUp, BUTTON_SINGLE_CLICK, nullptr, button_single_click_cb, &btnUp);

    constexpr button_config_t btnDownCfg = {0};
    const button_gpio_config_t btnDownGpioCfg = {
        .gpio_num = _downPin,
        .active_level = 0,
    };
    if(ret = iot_button_new_gpio_device(&btnDownCfg, &btnDownGpioCfg, &btnDown); ret != ESP_OK) {
        ESP_LOGE(BUTTON_TAG, "Button DOWN create failed");
    }
    iot_button_register_cb(btnDown, BUTTON_SINGLE_CLICK, nullptr, button_single_click_cb, &btnDown);

    constexpr button_config_t btnSelectCfg = {0};
    const button_gpio_config_t btnSelectGpioCfg = {
        .gpio_num = _selectPin,
        .active_level = 0,
    };
    if(ret = iot_button_new_gpio_device(&btnSelectCfg, &btnSelectGpioCfg, &btnSelect); ret != ESP_OK) {
        ESP_LOGE(BUTTON_TAG, "Button SELECT create failed");
    }
    iot_button_register_cb(btnSelect, BUTTON_SINGLE_CLICK, nullptr, button_single_click_cb, &btnSelect);
}
