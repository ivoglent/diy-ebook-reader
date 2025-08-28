//
// Created by long.nguyenviet on 8/28/25.
//
#pragma once
#include "HomeScreen.h"
#include "Screen.h"
#include "service.h"
#include "components/system/EventBus.h"
#include "components/system/Registry.h"

class ScreenManager: public BaseService<ScreenManager, SERVICE_SCREEN>, public EventSubscriber<ScreenManager, ButtonEvent>
{
private:
    std::vector<screen_t> _screens;
    std::string _currentScreen = HomeScreenName;
    screen_t getCurrentScreen();
public:
    explicit ScreenManager(Registry& registry);
    ~ScreenManager() override = default;
    void setup() override;
    void eventHandle(const ButtonEvent& event);
    void setCurrentScreen(const std::string& string);
};

inline void ScreenManager::setCurrentScreen(const std::string& name)
{
    _currentScreen = name;
    auto [screen] = getCurrentScreen();
    screen->load();
}

