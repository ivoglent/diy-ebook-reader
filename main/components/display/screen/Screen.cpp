//
// Created by long.nguyenviet on 8/28/25.
//

#include "Screen.h"


#include "ScreenManager.h"

Screen::Screen(ScreenManager* manager): _manager(manager)
{
    _data = lv_obj_create(nullptr);
    lv_obj_set_size(_data, SCREEN_WIDTH, SCREEN_HEIGHT);
}

void Screen::load()
{
    this->refresh();
    lv_scr_load(_data);
    lv_refr_now(nullptr);
}

void Screen::bringToFront()
{
    _manager->setCurrentScreen(getId());
}

void Screen::bringToFront(const std::string& id) const
{
    _manager->setCurrentScreen(id);
}
