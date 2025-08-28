//
// Created by long.nguyenviet on 8/28/25.
//

#include "ReadingScreen.h"


static bookinfo_t* book = nullptr;

std::string page_filename_snprintf(uint pageNum) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "page-%04d.bmp", pageNum);
    return buf;
}

static void handleReadingMenu(const ButtonEvent& event, Screen* screen)
{
    const auto home = static_cast<ReadingScreen*>(screen);
    home->onButtonEvent(event);
}


ReadingScreen::ReadingScreen(ScreenManager* manager): Screen(manager)
{
}

void ReadingScreen::init()
{
    if (book != nullptr)
    {
        auto filename = page_filename_snprintf(book->index);
        auto path = BOOK_BASE_PATH + book->title + "/" + filename;
        display_image_from_path(path);
    }
}


void ReadingScreen::refresh()
{
    global_current_book(book);
    init();
}

ScreenMenuEventHandle_t ReadingScreen::getMenuHandle()
{
    return handleReadingMenu;
}

std::string ReadingScreen::getId()
{
    return READING_SCREEN_NAME;
}

void ReadingScreen::onButtonEvent(const ButtonEvent& event)
{
    if (book == nullptr)
    {
        ESP_LOGE(READING_TAG, "No book selected");
        return;
    }
    switch (event.type)
    {
    case BUTTON_UP:
        book->index--;
        if (book->index < 1)
        {
            book->index = 1;
        } else
        {
            init();
        }
        break;
    case BUTTON_DOWN:
        book->index++;
        init();
        break;
    case BUTTON_SELECT:
        bringToFront(HomeScreenName);
        break;
    }
}
