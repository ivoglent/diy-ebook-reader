//
// Created by ivoglent on 8/24/2025.
//

#include "Button.h"

#include "esp_log.h"
#include "../system/EventBus.h"

static const char* BUTTON_TAG = "Button";
button_handle_t btnUp = nullptr;
button_handle_t btnDown = nullptr;
button_handle_t btnSelect = nullptr;
static void button_up_single_click_cb(void *arg,void *usr_data)
{
    const auto button = static_cast<Button*>(usr_data);
    button->handle(PRESSED, button->getUpPin());
}

static void button_down_single_click_cb(void *arg,void *usr_data)
{
    const auto button = static_cast<Button*>(usr_data);
    button->handle(PRESSED, button->getDownPin());
}

static void button_select_single_click_cb(void *arg,void *usr_data)
{
    const auto button = static_cast<Button*>(usr_data);
    button->handle(PRESSED, button->getSelectPin());
}

Button::Button(Registry& registry, uint8_t upPin, uint8_t downPin, uint8_t selectPin): BaseService(registry),
                                                                   _upPin(static_cast<gpio_num_t>(upPin)),
                                                                   _downPin(static_cast<gpio_num_t>(downPin)),
                                                                   _selectPin(static_cast<gpio_num_t>(selectPin)) {
}

void Button::setup() {
    esp_err_t ret;
    constexpr button_config_t btnUpCfg = {0};
    const button_gpio_config_t btnUpGpioCfg = {
        .gpio_num = _upPin,
        .active_level = 0,
    };
    if(ret = iot_button_new_gpio_device(&btnUpCfg, &btnUpGpioCfg, &btnUp); ret != ESP_OK) {
        ESP_LOGE(BUTTON_TAG, "Button UP create failed");
    }
    iot_button_register_cb(btnUp, BUTTON_SINGLE_CLICK, nullptr, button_up_single_click_cb, this);

    constexpr button_config_t btnDownCfg = {0};
    const button_gpio_config_t btnDownGpioCfg = {
        .gpio_num = _downPin,
        .active_level = 0,
    };
    if(ret = iot_button_new_gpio_device(&btnDownCfg, &btnDownGpioCfg, &btnDown); ret != ESP_OK) {
        ESP_LOGE(BUTTON_TAG, "Button DOWN create failed");
    }
    iot_button_register_cb(btnDown, BUTTON_SINGLE_CLICK, nullptr, button_down_single_click_cb, this);

    constexpr button_config_t btnSelectCfg = {0};
    const button_gpio_config_t btnSelectGpioCfg = {
        .gpio_num = _selectPin,
        .active_level = 0,
    };
    if(ret = iot_button_new_gpio_device(&btnSelectCfg, &btnSelectGpioCfg, &btnSelect); ret != ESP_OK) {
        ESP_LOGE(BUTTON_TAG, "Button SELECT create failed");
    }
    iot_button_register_cb(btnSelect, BUTTON_SINGLE_CLICK, nullptr, button_select_single_click_cb, this);
}

void Button::handle(const ButtonAction &action, const gpio_num_t &gpio) const {
    ESP_LOGI(BUTTON_TAG, "BUTTON_SINGLE_CLICK: %d", gpio);
    ButtonType type = BUTTON_UP;
    if (gpio == _upPin) {
        type = BUTTON_UP;
    } else if (gpio == _downPin) {
        type = BUTTON_DOWN;
    } else if (gpio == _selectPin) {
        type = BUTTON_SELECT;
    }
    ButtonEvent buttonEvent(
        type,
        action
    );
    getBus()->emit(buttonEvent);
}

