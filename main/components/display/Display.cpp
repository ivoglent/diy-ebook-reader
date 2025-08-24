//
// Created by ivoglent on 8/23/2025.
//

#include "Display.h"


static const char *TAG = "EPD_LVGL";

static lv_disp_draw_buf_t draw_buf;
static lv_disp_t *disp_handle = nullptr;
static esp_timer_handle_t lvgl_tick_timer = nullptr;
static inline void delay_ms(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
static lv_obj_t *canvas = nullptr;


static void draw_bmp_from_spiffs(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open BMP: %s", path);
        return;
    }

    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;
    fread(&file_header, sizeof(file_header), 1, f);
    fread(&info_header, sizeof(info_header), 1, f);

    if (file_header.bfType != 0x4D42 || info_header.biBitCount != 1) {
        ESP_LOGE(TAG, "Not a valid 1-bit BMP");
        fclose(f);
        return;
    }

    int w = info_header.biWidth;
    int h = info_header.biHeight;
    int row_size = ((w + 31) / 32) * 4;   // rows padded to 4 bytes

    fseek(f, file_header.bfOffBits, SEEK_SET);

    // Allocate LVGL buffer for full screen
    size_t buf_px = EPD_HOR_RES * EPD_VER_RES;
    static lv_color_t *lv_buf = NULL;
    if (!lv_buf) lv_buf = static_cast<lv_color_t *>(heap_caps_malloc(buf_px, MALLOC_CAP_SPIRAM));
    memset(lv_buf, 0xFF, buf_px); // all white

    uint8_t *row = static_cast<uint8_t *>(malloc(row_size));

    // read bottom→top
    for (int y = 0; y < h; y++) {
        fread(row, 1, row_size, f);
        for (int x = 0; x < w; x++) {
            int byte_idx = x / 8;
            int bit_idx  = 7 - (x % 8);
            int pixel    = (row[byte_idx] >> bit_idx) & 1;

            int draw_x = x;
            int draw_y = h - 1 - y;

            if (draw_x < EPD_HOR_RES && draw_y < EPD_VER_RES) {
                // LVGL 1bpp: .full == 0 → black, 1 → white
                lv_buf[draw_y * EPD_HOR_RES + draw_x].full = pixel ? 0 : 1;
            }
        }
    }

    free(row);
    fclose(f);

    // Create LVGL canvas object if not already
    if (!canvas) {
        canvas = lv_canvas_create(lv_scr_act());
        lv_obj_set_size(canvas, EPD_HOR_RES, EPD_VER_RES);
    }

    lv_canvas_set_buffer(canvas, lv_buf, EPD_HOR_RES, EPD_VER_RES, LV_IMG_CF_TRUE_COLOR);
    lv_obj_center(canvas);

    ESP_LOGI(TAG, "BMP %dx%d loaded into LVGL", w, h);
}


static void power_on_display(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << GPIO_DISPLAY_POWER_ON,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_set_level(GPIO_DISPLAY_POWER_ON, 1);
}

// =============== LVGL tick ===============
static void lvgl_tick_cb(void *arg) {
    // 5ms tick
    lv_tick_inc(5);
}

static void lvgl_start_tick_timer(void) {
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lvgl_tick_cb,
        .name = "lvgl_tick"
    };
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &lvgl_tick_timer));
    // 5ms period
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, 5000));
}

// =============== LVGL flush (full screen only) ===============
static void epd_lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    // Only support full-screen refresh for this EPD driver
    if (area->x1 != 0 || area->y1 != 0 || area->x2 != (EPD_HOR_RES - 1) || area->y2 != (EPD_VER_RES - 1)) {
        ESP_LOGW(TAG, "Partial flush requested (%d,%d)-(%d,%d) not supported, skipping.",
                 area->x1, area->y1, area->x2, area->y2);
        lv_disp_flush_ready(disp_drv);
        return;
    }

    const int width = EPD_HOR_RES;
    const int height = EPD_VER_RES;

    const size_t bytes_epd = (width * height) / 8; // 1 bit per pixel in EPD memory
    uint8_t *epd_buf = (uint8_t *)heap_caps_malloc(bytes_epd, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!epd_buf) {
        ESP_LOGE(TAG, "Failed to allocate %u bytes for EPD buffer", (unsigned)bytes_epd);
        lv_disp_flush_ready(disp_drv);
        return;
    }

    memset(epd_buf, 0xFF, bytes_epd); // White = 1s on SSD1677

    // Convert LVGL buffer (1 byte per pixel: 0=black, 1=white) -> packed 1bpp for SSD1677
    // LVGL delivers the area buffer contiguous left->right, top->bottom
    // We assume MSB is leftmost pixel per SSD1677 byte

#ifndef SWAP_XY
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const int pixel_index = y * width + x; // index in LVGL buffer
            const int byte_index  = (y * (width / 8)) + (x / 8);
            const int bit_index   = 7 - (x % 8);

            // In 1bpp LVGL, lv_color_t is 1 byte. value 0 = black, 1 = white.
            if (color_p[pixel_index].full == 0) { // black pixel
                epd_buf[byte_index] &= ~(1 << bit_index);
            }
        }
    }
#else
    // If your physical orientation is rotated (swap X/Y), use this path
    // This maps LVGL (width x height) into EPD's transposed memory
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const int src_index = y * width + x;      // LVGL index
            // transpose: (x,y) -> (y,x)
            const int tx = y;
            const int ty = x;
            const int byte_index  = (ty * (height / 8)) + (tx / 8);
            const int bit_index   = 7 - (tx % 8);
            if (color_p[src_index].full == 0) {
                epd_buf[byte_index] &= ~(1 << bit_index);
            }
        }
    }
#endif

    // Send to panel (blocking full refresh)
    EPD_WhiteScreen_ALL(epd_buf);

    free(epd_buf);
    lv_disp_flush_ready(disp_drv);
}

// =============== LVGL init & display registration ===============
static void lvgl_epd_register_display(void) {
    lv_init();

    // Allocate LVGL "draw" buffers. LVGL expects size in *pixels*, not bytes.
    const size_t buffer_pixels = (size_t)EPD_HOR_RES * (size_t)EPD_VER_RES; // full frame

    // In 1bpp mode lv_color_t is 1 byte, so this allocates buffer_pixels bytes per buffer
    lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    ESP_ERROR_CHECK_WITHOUT_ABORT(buf1 ? ESP_OK : ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK_WITHOUT_ABORT(buf2 ? ESP_OK : ESP_ERR_NO_MEM);

    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buffer_pixels);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EPD_HOR_RES;
    disp_drv.ver_res = EPD_VER_RES;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = epd_lvgl_flush_cb;

    // For monochrome, these can help avoid color conversions
#if LVGL_VERSION_MAJOR >= 8
//disp_drv.color_format = LV_COLOR_FORMAT_RGB565;   // or RGB888 depending on your panel
#endif

    disp_handle = lv_disp_drv_register(&disp_drv);

    lvgl_start_tick_timer();

    ESP_LOGI(TAG, "LVGL display registered: %dx%d, color depth=%d", EPD_HOR_RES, EPD_VER_RES, LV_COLOR_DEPTH);
}

// =============== SPI + Panel init ===============
static void spi_display_init(spi_host_device_t host_id) {
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SPI_PIN_MOSI,
        .miso_io_num = SPI_PIN_MISO,
        .sclk_io_num = SPI_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64 * 1024 // big enough for EPD chunks
    };

    ESP_ERROR_CHECK(spi_bus_initialize(host_id, &bus_cfg, SPI_DMA_CH_AUTO));

    // Initialize panel HW & mode
    EPD_HW_Init(host_id);
    delay_ms(100);
    EPD_W21_Init_Modes(Init_Mode_X_3);
}

// =============== Handle task ===============
static void lvgl_handle_task(void *pv) {
    while (true) {
        // Let LVGL handle timers/animations (tick provided by esp_timer)
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(20)); // ~50 FPS service; actual EPD refresh is blocking in flush
    }
}


Display::Display(Registry& registry): BaseService(registry) {};

Display::~Display() = default;

void Display::setup() {
    power_on_display();
    // IMPORTANT: init SPI + EPD first, then LVGL
    spi_display_init(SPI_HOST_USING);
    lvgl_epd_register_display();
    // Small delay before first frame
    delay_ms(500);
    xTaskCreatePinnedToCore(lvgl_handle_task, "lvgl_handle_task", 1024 * 6, NULL, 3, NULL, 0);
}

void Display::display(const std::string &path) {
    ESP_LOGI(TAG, "Drawing BMP from SPIFFS...");
    draw_bmp_from_spiffs(path.c_str());
}
