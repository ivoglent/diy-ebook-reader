//
// Created by ivoglent on 8/23/2025.
//

#pragma once
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_timer.h>
#include "esp_log.h"

// ===== LVGL config (ensure lv_conf.h matches these at build time) =====
#define LV_CONF_INCLUDE_SIMPLE
#include "lv_conf.h"
#ifndef LV_COLOR_DEPTH
#  error "lv_conf.h must define LV_COLOR_DEPTH"
#endif
#if LV_COLOR_DEPTH != 1
#  error "LV_COLOR_DEPTH must be 1 for monochrome EPD"
#endif
#include "lvgl.h"
//#include "components/fonts/roboto/roboto_18.c"
//#include "components/fonts/roboto/roboto_20.c"
#include <string>

#include "service.h"
#include "components/system/Registry.h"
#include "fonts/roboto/roboto_24.c"
//LV_FONT_DECLARE(roboto_18)
LV_FONT_DECLARE(roboto_24)

// ===== EPD driver headers =====
#include "GDEQ426T82/Display_EPD_W21_driver.h"


// ===== Pins / Power =====
#define GPIO_DISPLAY_POWER_ON   GPIO_NUM_38

// ===== SPI =====
#define SPI_HOST_USING          SPI2_HOST
#define SPI_PIN_MOSI            18
#define SPI_PIN_MISO            -1
#define SPI_PIN_SCLK            19
// ===== Panel Resolution (from driver) =====
#define EPD_HOR_RES             EPD_WIDTH   // expected 800
#define EPD_VER_RES             EPD_HEIGHT  // expected 480

// Optional: swap if your driver defines portrait by default
#define SWAP_XY 1


typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} __attribute__((packed)) BITMAPFILEHEADER;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} __attribute__((packed)) BITMAPINFOHEADER;

class Display: public BaseService<Display, SERVICE_DISPLAY> {
private:

public:
    Display(Registry& registry);
    ~Display();
    void setup();
    void display(const std::string& path);
};

void display_partially(const lv_area_t *area, lv_color_t *lv_buf);
lv_disp_draw_buf_t display_get_draw_buff();
void display_image_from_path(const std::string& path);