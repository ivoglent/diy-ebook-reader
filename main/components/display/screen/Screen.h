//
// Created by long.nguyenviet on 8/28/25.
//
#pragma once
#include <functional>
#include <string>
#include <core/lv_obj.h>
#include <core/lv_disp.h>
#include "event.h"
class Screen;
class ScreenManager;
typedef std::function<void(const ButtonEvent& event, Screen* self)> ScreenMenuEventHandle_t;

class Screen {
private:
    ScreenManager* _manager;
protected:
    lv_obj_t* _data = nullptr;
public:
    explicit Screen(ScreenManager* manager);
    virtual ~Screen() = default;
    virtual std::string getId();
    void load();
    virtual void init();
    virtual void refresh();
    virtual ScreenMenuEventHandle_t getMenuHandle();
    void bringToFront();
    void bringToFront(const std::string& id) const;
};


typedef struct
{
    Screen* screen;
} screen_t;