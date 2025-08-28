//
// Created by long.nguyenviet on 8/28/25.
//
#pragma once
#include "Screen.h"


class SettingScreen: public Screen {
public:
    SettingScreen(ScreenManager* manager);
    ~SettingScreen() override = default;
    void init() override;
    void refresh() override;
    ScreenMenuEventHandle_t getMenuHandle() override;
    std::string getId() override;
    void onButtonEvent(const ButtonEvent& event);
};

