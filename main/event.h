//
// Created by ivoglent on 8/24/2025.
//

#pragma once
#include "esp_event.h"

ESP_EVENT_DEFINE_BASE(USER_EVENTS);

typedef enum {
    EVENT_SYSTEM_READY = 1,
    EVENT_BUTTON
} user_event_id_t;

template<int EVT_ID>
struct Event {
    static constexpr int32_t ID = EVT_ID;
    virtual ~Event() = default;
    int getId() const { return ID; }
};

typedef enum {
    BUTTON_UP = 1,
    BUTTON_DOWN,
    BUTTON_SELECT
} ButtonType;

typedef enum {
    PRESSED = 1,
    LONG_PRESSED,
    HELD
} ButtonAction;

struct ButtonEvent : Event<EVENT_BUTTON> {
    ButtonType type;
    ButtonAction action;
    ButtonEvent(const ButtonType& t, const ButtonAction& a) : type(t), action(a) {}
};