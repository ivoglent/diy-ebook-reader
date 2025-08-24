#pragma once

#include <utility>
#include <vector>

#include "EventBus.h"

#include <freertos/FreeRTOS.h>
#include <memory>


typedef uint16_t ServiceId;
typedef uint8_t ServiceSubId;
typedef uint8_t SystemId;

class Registry;


class Service
{
public:
    typedef std::shared_ptr<Service> Ptr;
    [[nodiscard]] virtual ServiceId getServiceId() const = 0;

    virtual Registry& getRegistry() = 0;

    virtual void setup()
    {
    }

    virtual void subscribe()
    {
    }

    virtual void loop()
    {
    }

    virtual void destroy()
    {
    }

    virtual ~Service() = default;
};

typedef std::vector<Service*> ServiceArray;

class Registry
{
    ServiceArray _services;
    Service* doGetService(ServiceId serviceId)
    {
        for (auto& service : getServices())
        {
            if (service->getServiceId() == serviceId)
            {
                return service;
            }
        }

        return nullptr;
    }

public:
    Registry() = default;
    Registry(const Registry&) = delete;
    Registry(Registry&&) = delete;

    template <typename C, typename... T>
    C& create(T&&... all)
    {
        C* service = new C(*this, std::forward<T>(all)...);
        _services.push_back(service); // Use std::unique_ptr for ownership
        return *service;
    }


    template <typename C>
    C* getService()
    {
        //esp_logd(reg, "Querying service...:%d, total service:%d", C::ID, _services.size());
        return static_cast<C*>(doGetService(C::ID));
    }

    ServiceArray& getServices()
    {
        return _services;
    }

};

static bool is_in_isr()
{
    return xPortInIsrContext();
}

template <typename T, ServiceSubId Id>
class BaseService : public Service, public std::enable_shared_from_this<T>
{
    Registry& _registry;
    static T* instance;

public:
    explicit BaseService(Registry& registry) : _registry(registry)
    {
    }

    enum
    {
        ID = Id
    };

    [[nodiscard]] ServiceId getServiceId() const override
    {
        return Id;
    }

    Registry& getRegistry() override
    {
        return _registry;
    }

    void subscribe() override
    {
        using Subscriber = typename SubscriberTraits<T>::SubscriberType;
        if constexpr (!std::is_same_v<Subscriber, void>) {
            static_cast<Subscriber*>(this)->registerEvents();
        }
        else
        {
            ESP_LOGW("REG", "Not subscribe, not extended from EventSubscriber: %d", ID);
        }
    }

    void setup() override
    {
    }
};

Registry& getRegistryInstance();
