//
// Created by long.nguyenviet on 8/28/25.
//

#include "Screen.h"


#include "ScreenManager.h"

Screen::Screen(ScreenManager* manager): _manager(manager)
{

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
