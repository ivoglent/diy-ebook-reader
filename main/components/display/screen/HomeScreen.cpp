//
// Created by long.nguyenviet on 8/28/25.
//

#include "HomeScreen.h"

#include "ReadingScreen.h"
#include "components/display/Display.h"
#include "components/sdcard/SDCard.h"
#include "components/system/global.h"
#include "dto/bookinfo.h"


static lv_obj_t *book_items[ITEMS_PER_PAGE];
static lv_obj_t *book_labels[ITEMS_PER_PAGE];
static int selected_index = -1;
static int current_page = 0;
static int total_items = 0;
static const char **all_titles = nullptr;



static void refresh_item_area(const lv_obj_t *obj) {
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    // Get pointer to current display buffer (from draw_buf or active screen)
    // Easiest way is to render the object to an image buffer.
    // BUT if your flush_cb already works, you can pass lv_disp_get_draw_buf
    const auto buf = static_cast<lv_color_t*>(display_get_draw_buff().buf_act);  // assuming you made draw_buf global
    display_partially(&coords, buf);
}

static void create_book_grid(lv_obj_t *parent, char *titles[], int count) {
    const int page_items = (count < ITEMS_PER_PAGE) ? count : ITEMS_PER_PAGE;

    for (int i = 0; i < page_items; i++) {
        const int col = i % PAGE_COLS;
        const int row = i / PAGE_COLS;

        const int x = ITEM_MARGIN + col * (ITEM_W + ITEM_MARGIN);
        const int y = ITEM_MARGIN + row * (ITEM_H + ITEM_MARGIN);

        // Box
        book_items[i] = lv_obj_create(parent);
        lv_obj_set_size(book_items[i], ITEM_W, ITEM_H);
        lv_obj_set_pos(book_items[i], x, y);
        lv_obj_set_style_border_width(book_items[i], 1, 0);
        lv_obj_set_style_border_color(book_items[i], lv_color_black(), 0);
        lv_obj_set_style_bg_color(book_items[i], lv_color_white(), 0);
        lv_obj_set_style_radius(book_items[i], 0, 0);

        // Title
        book_labels[i] = lv_label_create(book_items[i]);
        lv_label_set_text(book_labels[i], titles[i]);
        lv_obj_center(book_labels[i]);
    }
}

static void set_selected(const int new_index) {
    if (selected_index >= 0 && book_items[selected_index]) {
        lv_obj_set_style_border_width(book_items[selected_index], 1, 0); // unselect
        refresh_item_area(book_items[selected_index]);
    }
    selected_index = new_index;
    lv_obj_set_style_border_width(book_items[selected_index], 3, 0);     // select
    refresh_item_area(book_items[selected_index]);
}

static void show_page(const int page, lv_obj_t *parent) {
    lv_obj_clean(parent); // remove old items
    current_page = page;

    const int start = page * ITEMS_PER_PAGE;
    const int remain = total_items - start;
    const int page_count = (remain < ITEMS_PER_PAGE) ? remain : ITEMS_PER_PAGE;

    for (int i = 0; i < page_count; i++) {
        const int col = i % PAGE_COLS;
        const int row = i / PAGE_COLS;
        const int x = ITEM_MARGIN + col * (ITEM_W + ITEM_MARGIN);
        const int y = ITEM_MARGIN + row * (ITEM_H + ITEM_MARGIN);

        book_items[i] = lv_obj_create(parent);
        lv_obj_set_size(book_items[i], ITEM_W, ITEM_H);
        lv_obj_set_pos(book_items[i], x, y);
        lv_obj_set_style_border_width(book_items[i], 1, 0);
        lv_obj_set_style_border_color(book_items[i], lv_color_black(), 0);

        book_labels[i] = lv_label_create(book_items[i]);
        lv_label_set_text(book_labels[i], all_titles[start + i]);
        lv_obj_center(book_labels[i]);
    }
}

static void move_selection(int delta) {
    const int page_start = current_page * ITEMS_PER_PAGE;
    const int page_end   = page_start + ITEMS_PER_PAGE - 1;
    const int new_index  = selected_index + delta;

    if (new_index < page_start) {
        // prev page
        if (current_page > 0) {
            show_page(current_page - 1, lv_scr_act());
            set_selected((current_page - 1) * ITEMS_PER_PAGE + (ITEMS_PER_PAGE - 1));
        }
        return;
    } else if (new_index > page_end) {
        // next page
        if ((current_page + 1) * ITEMS_PER_PAGE < total_items) {
            show_page(current_page + 1, lv_scr_act());
            set_selected((current_page + 1) * ITEMS_PER_PAGE);
        }
        return;
    }

    // same page selection
    set_selected(new_index);
}


static void handleHomeMenu(const ButtonEvent& event, Screen* screen)
{
    const auto home = static_cast<HomeScreen*>(screen);
    home->onButtonEvent(event);
}

int HomeScreen::_loadDataFromSdCard(char** items)
{
    auto dirs = list_directories("/sdcard/books");
    uint size = MAX_BOOK_ITEMS;
    if (size > dirs.size())
    {
        size = dirs.size();
    }
    for (int i = 0; i < size; i++)
    {
        strcpy(items[i] , dirs[i].c_str());
    }
    return dirs.size();
}

HomeScreen::HomeScreen(ScreenManager* manager): Screen(manager)
{
}

void HomeScreen::init()
{
    char* items[MAX_BOOK_ITEMS];
    int total = _loadDataFromSdCard(items);
    _data = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(_data, lv_color_white(), 0);
    create_book_grid(_data, items, total);
}

void HomeScreen::refresh()
{
}

ScreenMenuEventHandle_t HomeScreen::getMenuHandle()
{
    return handleHomeMenu;
}

std::string HomeScreen::getId()
{
    return HomeScreenName;
}

void HomeScreen::onButtonEvent(const ButtonEvent& event)
{
    switch (event.type)
    {
        case BUTTON_UP:
            ESP_LOGI(HOME_TAG, "[Home] Button Up");
            selected_index--;
            move_selection(selected_index);
        break;
        case BUTTON_DOWN:
            ESP_LOGI(HOME_TAG, "[Home] Button Down");
            selected_index++;
            move_selection(selected_index);
            break;
        case BUTTON_SELECT:
            ESP_LOGI(HOME_TAG, "[Home] Button Select");
            _selectedBook(all_titles[selected_index]);
        break;
    }
}

void HomeScreen::_selectedBook(const std::string& name)
{
    std::string path = BOOK_BASE_PATH + name;
    auto json = read_string_file(path.c_str());
    if (json != nullptr)
    {
        bookinfo_t bookInfo{};
        parse_bookinfo(json, bookInfo);
        global_set_current_book(&bookInfo);
        this->bringToFront(READING_SCREEN_NAME);
    }
}
