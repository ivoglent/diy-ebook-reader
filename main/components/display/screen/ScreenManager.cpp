//
// Created by long.nguyenviet on 8/28/25.
//

#include "ScreenManager.h"

#include "HomeScreen.h"
#include "ReadingScreen.h"
#include "SettingScreen.h"

screen_t ScreenManager::getCurrentScreen()
{
    for (auto screen : _screens)
    {
        if (screen.screen->getId() == _currentScreen)
        {
            return screen;
        }
    }
    return _screens.front();
}

ScreenManager::ScreenManager(Registry& registry): BaseService(registry)
{
}

void ScreenManager::setup()
{
    auto* home = new HomeScreen(this);
    home->init();
    _screens.push_back({ home });

    auto* reading = new ReadingScreen(this);
    reading->init();
    _screens.push_back({ reading });

    auto* setting = new SettingScreen(this);
    setting->init();
    _screens.push_back({ setting });

    this->setCurrentScreen(HomeScreenName);
}

void ScreenManager::eventHandle(const ButtonEvent& event)
{
    auto [screen] = getCurrentScreen();
    screen->getMenuHandle()(event, screen);
}
