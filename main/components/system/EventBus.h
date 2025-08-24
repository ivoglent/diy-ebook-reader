//
// Created by ivoglent on 8/24/2025.
//

#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <functional>

#include "esp_err.h"
#include "portmacro.h"
#include "esp_log.h"
#include "../../event.h"


#define EVENT_TAG "EVT"

using EventCallback = void(*)(void*);
struct CallbackEntry {
    void* subscriber;
    void (*callback)(void* subscriber, void* msg);
};
template <typename T>
using SubscriberCallback = std::function<void(const T& msg)>;

class EventBus {
public:
    template<typename Subscriber, typename EventType>
    void registerEvent(Subscriber* sub) {
        CallbackEntry cb(
            sub,
            &EventBus::dispatch<Subscriber, EventType>
        );
        callbacks[EventType::ID].push_back(cb);
        ESP_LOGI(EVENT_TAG, "Registered event %ld", EventType::ID);
    }

    template<typename Subscriber, typename EventType>
    static void dispatch(void* subscriber, void* msg) {
        static_cast<Subscriber*>(subscriber)->eventHandle(*reinterpret_cast<EventType*>(msg));
    }

    template<typename EventType>
    void emit(EventType& ev) {
        ESP_ERROR_CHECK(esp_event_post(USER_EVENTS, EventType::ID, &ev, sizeof(ev), portMAX_DELAY));
    }

    void onMessage(const int32_t& id, void* msg);

private:
    std::unordered_map<int, std::vector<CallbackEntry>> callbacks;
};


esp_err_t event_bus_init();
EventBus* getBus();




class IEventSubscriber {
public:
    virtual ~IEventSubscriber() = default;

    // Every subscriber must provide its IDs
    virtual std::vector<uint32_t> getIDs() const = 0;

    virtual void onEvent(void* msg, uint32_t id) = 0;

    virtual void registerEvents() = 0;
};


template <typename T, typename... Msgs>
class EventSubscriber: public IEventSubscriber
{
public:
    std::vector<uint32_t> getIDs() const override
    {
        return { Msgs::ID... };
    }

    void onEvent(void* msg, uint32_t id) override {
        if (const bool handled = (tryHandle<Msgs>(msg, id) || ...); !handled) {
            ESP_LOGW("BUS", "Unhandled event ID: %ld", id);
        }
    }

    void registerEvents() override {
        T* self = static_cast<T*>(this);
        (getBus()->registerEvent<T, Msgs>(self), ...);
    }

private:
    template <typename Msg>
    bool tryHandle(void* msg, uint32_t id) {
        if (Msg::ID == id) {
            static_cast<T*>(this)->eventHandle(*reinterpret_cast<Msg*>(msg));
            return true;
        }
        return false;
    }
};

// Primary template: default case (not an EventSubscriber)
template <typename T, typename = void>
struct SubscriberTraits {
    using SubscriberType = void; // fallback
};

// Specialization: T inherits from EventSubscriber
template <typename T>
struct SubscriberTraits<T, std::void_t<decltype(&T::registerEvents)>> {
    using SubscriberType = T; // T has registerEvents()
};