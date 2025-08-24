//
// Created by ivoglent on 8/24/2025.
//

#include "Menu.h"

static int pageNumber = 1;
Menu::Menu(Registry &registry): BaseService(registry)  {
}

void Menu::eventHandle(const ButtonEvent &event) {
    ESP_LOGI("MENU", "Got button event");
    if (event.type == BUTTON_UP) {
        pageNumber++;
    } else if (event.type == BUTTON_DOWN) {
        pageNumber--;
    }
    if (pageNumber < 1) {
        pageNumber = 1;
    }
    if (pageNumber > 9) {
        pageNumber = 9;
    }
    std::stringstream ss;
    ss << "/spiffs/pages/page_00";
    ss << pageNumber;
    ss << ".bmp";
    auto display = getRegistryInstance().getService<Display>();
    display->display(ss.str());
    ESP_LOGI("MENU", "Displaying page number: %d", pageNumber);
}