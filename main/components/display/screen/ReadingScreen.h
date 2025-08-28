//
// Created by long.nguyenviet on 8/28/25.
//

#pragma once
#include "Screen.h"
#include "dto/bookinfo.h"
#include "components/system/global.h"
#include <sstream>
#include "components/display/Display.h"
#include "HomeScreen.h"

#define READING_TAG "READING"

#define READING_SCREEN_NAME "ReadingScreen"

class ReadingScreen: public Screen {
public:
    ReadingScreen(ScreenManager* manager);
    ~ReadingScreen() override = default;
    void init() override;
    void refresh() override;
    ScreenMenuEventHandle_t getMenuHandle() override;
    std::string getId() override;
    void onButtonEvent(const ButtonEvent& event);
};


