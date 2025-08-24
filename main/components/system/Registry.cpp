#include "Registry.h"
static std::unique_ptr<Registry> instance = nullptr;

Registry& getRegistryInstance()
{
    if (instance == nullptr)
    {
        ESP_LOGI("REG", "Registry instance empty, creating new");
        instance = std::make_unique<Registry>();
    }
    return *instance;
};
