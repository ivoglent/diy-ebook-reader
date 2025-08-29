//
// Created by ivoglent on 8/24/2025.
//

#pragma once
#include <cstddef>
#include "esp_err.h"
#include "iot_button.h"
#include "button_gpio.h"
#include "service.h"
#include "components/system/Registry.h"
#include "soc/gpio_num.h"
#include "console.h"

typedef struct {
    void* button;
    gpio_num_t gpio;
    ButtonAction action;
} ButtonArgs;

class Button: public BaseService<Button, SERVICE_BUTTON> {
private:
    gpio_num_t _upPin;
    gpio_num_t _downPin;
    gpio_num_t _selectPin;
public:
    Button(Registry& registry, uint8_t upPin, uint8_t downPin, uint8_t selectPin);
    ~Button() override = default;
    void setup() override;
    gpio_num_t getUpPin() {
        return _upPin;
    }
    gpio_num_t getDownPin() {
        return _downPin;
    }
    gpio_num_t getSelectPin() {
        return _selectPin;
    }
    void handle(const ButtonAction& action, const gpio_num_t& gpio) const;
};

