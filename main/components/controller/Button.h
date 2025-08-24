//
// Created by ivoglent on 8/24/2025.
//

#pragma once
#include <cstddef>
#include "esp_err.h"
#include "iot_button.h"
#include "button_gpio.h"
#include "soc/gpio_num.h"

class Button {
private:
    gpio_num_t _upPin;
    gpio_num_t _downPin;
    gpio_num_t _selectPin;
public:
    Button(uint8_t upPin, uint8_t downPin, uint8_t selectPin);
    ~Button() = default;
    void setup() const;
};
