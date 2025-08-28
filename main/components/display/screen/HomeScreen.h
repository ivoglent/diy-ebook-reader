//
// Created by long.nguyenviet on 8/28/25.
//

#pragma once
#include "Screen.h"
#include <esp_log.h>
#include <widgets/lv_label.h>


#define HomeScreenName "HOME"
#define HOME_TAG "HomeScreen"
#define ITEM_W 100
#define ITEM_H 100
#define ITEM_MARGIN 10
#define PAGE_COLS 4
#define PAGE_ROWS 7
#define ITEMS_PER_PAGE (PAGE_COLS * PAGE_ROWS)
#define MAX_BOOK_ITEMS 1000

class HomeScreen final : public Screen {
private:
    int _loadDataFromSdCard(char** items);
    void _selectedBook(const std::string& name);
public:
    HomeScreen(ScreenManager* manager);
    ~HomeScreen() override = default;

    void init() override;
    void refresh() override;
    ScreenMenuEventHandle_t getMenuHandle() override;
    std::string getId() override;
    void onButtonEvent(const ButtonEvent& event);
};


