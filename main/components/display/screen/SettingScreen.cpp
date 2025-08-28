//
// Created by long.nguyenviet on 8/28/25.
//

#include "SettingScreen.h"

static void handleSettingMenu(const ButtonEvent& event, Screen* screen)
{
    const auto self = static_cast<SettingScreen*>(screen);
    self->onButtonEvent(event);
}

SettingScreen::SettingScreen(ScreenManager* manager): Screen(manager)
{
}

void SettingScreen::init()
{

}

void SettingScreen::refresh()
{
}

ScreenMenuEventHandle_t SettingScreen::getMenuHandle()
{
    return handleSettingMenu;
}

std::string SettingScreen::getId()
{
    return SETTING_SCREEN_NAME;
}

void SettingScreen::onButtonEvent(const ButtonEvent& event)
{

}
