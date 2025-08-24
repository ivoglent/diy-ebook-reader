//
// Created by ivoglent on 8/24/2025.
//

#include "EventBus.h"

static EventBus* eventBus;

static void user_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    ESP_LOGI(EVENT_TAG, "Received event with id: %ld", id);
    eventBus->onMessage(id, event_data);
}

void EventBus::onMessage(const int32_t &id, void *msg) {
    bool handled = false;
    auto it = callbacks.find(id);
    if(it != callbacks.end()) {
        for(const auto& cb : it->second) {
            cb.callback(cb.subscriber, msg); // pass the pointer itself, not its address
            handled = true;
        }
    }
    if(!handled) {
        ESP_LOGI(EVENT_TAG, "Event not handled id: %ld", id);
    }
}

esp_err_t event_bus_init()  {
    eventBus = new EventBus();
    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Register handler for custom events
    ESP_ERROR_CHECK(esp_event_handler_register(USER_EVENTS, ESP_EVENT_ANY_ID, &user_event_handler, eventBus));

    return ESP_OK;
}

EventBus* getBus() {
    return eventBus;
}


