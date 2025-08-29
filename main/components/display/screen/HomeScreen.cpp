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
static int selected_index = 0;
static int current_page = 0;
static int total_items = 0;
static const char **all_titles = nullptr;
static std::vector<std::string> book_titles;

static lv_area_t s_partial_area;
static void refresh_item_area(const lv_obj_t *obj) {
    lv_obj_get_coords(obj, &s_partial_area);

    // Align to 8 pixels horizontally for byte packing
    s_partial_area.x1 &= ~0x7;
    s_partial_area.x2 |= 0x7;

    // Ask LVGL to redraw just this object
    lv_obj_invalidate(obj);
    lv_refr_now(nullptr);
}

static void create_book_grid(lv_obj_t *parent, const std::vector<std::string>& titles, int count) {
    static lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

    lv_obj_set_grid_dsc_array(parent, col_dsc, row_dsc);

    for (int i = 0; i < ITEMS_PER_PAGE && i < count; i++) {
        lv_obj_t *item = lv_obj_create(parent);

        lv_obj_set_grid_cell(item,
                             LV_GRID_ALIGN_STRETCH, i % 3, 1,
                             LV_GRID_ALIGN_STRETCH, i / 3, 1);

        // Style border
        static lv_style_t style_border;
        lv_style_init(&style_border);
        lv_style_set_border_color(&style_border, lv_color_black());
        lv_style_set_border_width(&style_border, 1);
        lv_obj_add_style(item, &style_border, 0);

        if (i == 0) {
            static lv_style_t style_selected;
            lv_style_init(&style_selected);
            lv_style_set_border_color(&style_selected, lv_color_black());
            lv_style_set_border_width(&style_selected, 3);
            lv_obj_add_style(item, &style_selected, 0);
        }

        // Label (at bottom with padding)
        lv_obj_t *label = lv_label_create(item);
        lv_label_set_text(label, titles[i].c_str());
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);   // wrap if long
        lv_obj_set_width(label, lv_pct(100));                // fit width

        lv_obj_align(label, LV_ALIGN_BOTTOM_LEFT, 10, -10);  // left + bottom padding
        book_items[i] = item;
    }
}




static void set_selected(const int new_index) {
    if (selected_index >= 0 && book_items[selected_index]) {
        lv_obj_set_style_border_width(book_items[selected_index], 1, 0); // unselect
        refresh_item_area(book_items[selected_index]);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    selected_index = new_index;
    lv_obj_set_style_border_width(book_items[selected_index], 3, 0);     // select
    refresh_item_area(book_items[selected_index]);
}

static void show_page(const int page, lv_obj_t *parent) {
    /*lv_obj_clean(parent); // remove old items
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
    }*/
}

static void move_selection(int delta) {
    /*const int page_start = current_page * ITEMS_PER_PAGE;
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
    }*/
    const uint new_index = selected_index + delta;
    // same page selection
    set_selected(new_index);
}


static void handleHomeMenu(const ButtonEvent& event, Screen* screen)
{
    const auto home = static_cast<HomeScreen*>(screen);
    home->onButtonEvent(event);
}

int HomeScreen::_loadDataFromSdCard(std::vector<std::string> items)
{
    auto dirs = list_directories("/sdcard/books");
    uint size = MAX_BOOK_ITEMS;
    if (size > dirs.size())
    {
        size = dirs.size();
    }
    for (int i = 0; i < size; i++)
    {
        items.emplace_back(dirs[i].c_str());
    }
    return dirs.size();
}

HomeScreen::HomeScreen(ScreenManager* manager): Screen(manager)
{
}

void HomeScreen::init()
{
    lv_obj_set_style_bg_color(_data, lv_color_white(), 0);
    book_titles = {
        "10 nghich ly cuoc song",
        "Ai Lay Mieng Pho Mat Cua Toi",
        "Ban Khong Thong Minh Lam Dau",
        "Doc vi bat ky ai",
        "Giua Nhuung Dieu Binh Di",
        "Nhuung Don Tam Ly Trong Thuyet Phuc",
        "Phi ly tri",
        "Sapiens: Luoc Su Loai Nguoi",
        "Sieu Tri Tue",
        "Suy Ngam Ve Thien va Ac",
        "Tam ly hoc dam dong",
        "Thai Do Quyet Dinh Thanh Cong",
        "Thuat-doc-nguoi",
        "Tu duy nhanh va chap",
        "Vol11",
        "Dieu Binh Di Thong Thai"
    };

    //int total = _loadDataFromSdCard(book_titles);
    int total = 16;
    selected_index = 0;
    create_book_grid(_data, book_titles, total);
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
            move_selection(-1);
        break;
        case BUTTON_DOWN:
            ESP_LOGI(HOME_TAG, "[Home] Button Down");
            move_selection(1);
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
