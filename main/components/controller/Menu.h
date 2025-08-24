//
// Created by ivoglent on 8/24/2025.
//
#pragma once
#include "service.h"
#include "components/system/Registry.h"
#include <sstream>
#include "components/display/Display.h"


class Menu: public BaseService<Menu, SERVICE_MENU>, public EventSubscriber<Menu, ButtonEvent>{

public:
    explicit Menu(Registry& registry);
    ~Menu() override = default;
    void eventHandle(const ButtonEvent& event);
};



