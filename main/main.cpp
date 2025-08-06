//
// Created by long.nguyenviet on 7/18/25.
//

#include <esp_lcd_types.h>
#include <esp_log.h>
#include <stdio.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define LV_CONF_INCLUDE_SIMPLE
#include <esp_task_wdt.h>

#include "lvgl.h"
#include "components/GDEQ426T82/Ap_29demo.h"
#include "components/GDEQ426T82/Display_EPD_W21_driver.h"
#define LV_COLOR_DEPTH     1
#define IS_COLOR_EPD 1

// Enable Power on Display
#define GPIO_DISPLAY_POWER_ON GPIO_NUM_38
#define BUF_PIXEL_COUNT (EPD_HOR_RES * EPD_VER_RES)


// Using SPI2 in the example
#define LCD_HOST  SPI2_HOST

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
#define EXAMPLE_PIN_NUM_SCLK           19
#define EXAMPLE_PIN_NUM_MOSI           18
#define EXAMPLE_PIN_NUM_MISO           (-1)   // Unused
#define EXAMPLE_PIN_NUM_EPD_DC         12
#define EXAMPLE_PIN_NUM_EPD_RST        5
#define EXAMPLE_PIN_NUM_EPD_CS         11
#define EXAMPLE_PIN_NUM_EPD_BUSY       10

#define SPI_PIN_MOSI GPIO_NUM_18
#define  SPI_PIN_MISO GPIO_NUM_NC
#define  SPI_PIN_SCLK GPIO_NUM_19

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              480
#define EXAMPLE_LCD_V_RES              800

// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define SPI_HOST_USING  SPI2_HOST


extern "C"
{
   void app_main();
}
void delay(uint32_t millis) { vTaskDelay(millis / portTICK_PERIOD_MS); }
void power_on()
{
   gpio_config_t io_conf = {
      .pin_bit_mask = 1ULL << GPIO_DISPLAY_POWER_ON,
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE
  };
   gpio_config(&io_conf);
   gpio_set_level(GPIO_DISPLAY_POWER_ON, 1);
}
#define EPD_HOR_RES EPD_WIDTH
#define EPD_VER_RES EPD_HEIGHT

static QueueHandle_t epd_flush_queue;

typedef struct {
   lv_area_t area;
   uint8_t *buf;
   lv_disp_drv_t *disp_drv;
} epd_flush_req_t;

static lv_disp_draw_buf_t draw_buf;
//static uint8_t *lv_buf1 = nullptr;
static uint8_t lv_buffer_mem[BUF_PIXEL_COUNT / 8] = {0};

void epd_flush_task(void *pvParameters)
{
   epd_flush_req_t req;
   while (1)
   {
      if (xQueueReceive(epd_flush_queue, &req, portMAX_DELAY))
      {
         // Call the blocking update
         EPD_WhiteScreen_ALL(req.buf);
         lv_disp_flush_ready(req.disp_drv);
         // Clean up
         free(req.buf);
      }
   }
}

// LVGL flush callback: gets called when LVGL wants to update an area of the screen
void my_lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
   // For simplicity, flush the whole screen (monochrome 1bpp)
   // color_p is assumed to be a full-screen 1-bit buffer
   ESP_LOGI("FLUSH", "Area: x(%d-%d), y(%d-%d), ptr: %p", area->x1, area->x2, area->y1, area->y2, color_p);
   ESP_LOG_BUFFER_HEXDUMP("LVGL_BUF", color_p, 16, ESP_LOG_INFO);
   EPD_WhiteScreen_ALL(reinterpret_cast<uint8_t*>(color_p));
   lv_disp_flush_ready(disp_drv);
   return;
   epd_flush_req_t req;
   req.area = *area;
   req.disp_drv = disp_drv;
   // Allocate a buffer copy
   size_t len = BUF_PIXEL_COUNT / 8;
   req.buf = (uint8_t *)heap_caps_malloc(len, MALLOC_CAP_DMA);
   memcpy(req.buf, (uint8_t *)color_p, len);

   xQueueSend(epd_flush_queue, &req, portMAX_DELAY);

   lv_disp_flush_ready(disp_drv);
}

void lvgl_epd_ssd1677_init(spi_host_device_t spi_host)
{
   lv_init();

   // Allocate buffer for LVGL
   //lv_buf1 = static_cast<uint8_t *>(heap_caps_malloc(BUF_PIXEL_COUNT / 8, MALLOC_CAP_DMA));
   //assert(lv_buf1);
   //ESP_LOGI("LVGL", "Using buffer %p (%d bytes)", lv_buffer_mem, sizeof(lv_buffer_mem));

   //lv_disp_draw_buf_init(&draw_buf, lv_buffer_mem, nullptr, BUF_PIXEL_COUNT);
   auto *buf1 = static_cast<lv_color_t*>(heap_caps_malloc(EXAMPLE_LCD_H_RES * 800 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM));
   assert(buf1);
   auto *buf2 = static_cast<lv_color_t*>(heap_caps_malloc(EXAMPLE_LCD_H_RES * 800 * sizeof(lv_color_t), MALLOC_CAP_SPIRAM));
   assert(buf2);
   // alloc bitmap buffer to draw
   //converted_buffer_black = heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES / 8, MALLOC_CAP_DMA);
   //converted_buffer_red = heap_caps_malloc(EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES / 8, MALLOC_CAP_DMA);
   // initialize LVGL draw buffers
   lv_disp_draw_buf_init(&draw_buf, buf1, buf2, EXAMPLE_LCD_H_RES * 800);

   static lv_disp_drv_t disp_drv;
   lv_disp_drv_init(&disp_drv);
   disp_drv.hor_res = EPD_HOR_RES;
   disp_drv.ver_res = EPD_VER_RES;
   disp_drv.flush_cb = my_lvgl_flush_cb;
   disp_drv.draw_buf = &draw_buf;
   //disp_drv.color_format = LV_COLOR_FORMAT_I1; // 1-bit monochrome

   lv_disp_drv_register(&disp_drv);

   ESP_LOGI("main", "LVGL EPD SSD1677 display initialized");
}

void lvgl_handle_task(void *pvParameters)
{
   while (1)
   {
      lv_timer_handler();
      vTaskDelay(pdMS_TO_TICKS(10));
   }
   vTaskDelete(NULL);
}

void __ESP32_SPI_Bus_Init(spi_host_device_t host_id)
{
   esp_err_t ret;

   spi_bus_config_t spiBusCfg = {
      .mosi_io_num = SPI_PIN_MOSI,
      .miso_io_num = SPI_PIN_MISO,
      .sclk_io_num = SPI_PIN_SCLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 160*800*2+8  //TODO: 这里太大了???
  };

   ret = spi_bus_initialize(host_id, &spiBusCfg, SPI_DMA_CH_AUTO);
   ESP_ERROR_CHECK(ret);
}
#define IMG_INDEX_MAX   EPD_ARRAY
uint8_t GUIImg2[IMG_INDEX_MAX]; //缓存界面
uint8_t *DispImg2; //一个bit存一个pix


void app_main(void)
{
   // This is just in case you power your epaper VCC with a GPIO
   power_on();
   //delay(100);
   __ESP32_SPI_Bus_Init(SPI_HOST_USING);
   // Init your EPD hardware
   EPD_HW_Init(SPI_HOST_USING);
   //EPD_W21_Init_GUI();

   vTaskDelay(pdMS_TO_TICKS(1000));

   DispImg2 = GUIImg2;

   //------------ 使用GUI, 并支持局部刷新
   //GUI_Init_Img(GUIImg2, EPD_X_LEN, EPD_Y_LEN, Rotate0, ColorWhite); //需要交换W和H
   //GUI_Clear_All(ColorWhite);
   //
   EPD_W21_Init_GUI();
   EPD_SetRAMValue_BaseMap(GUIImg2); //设置背景

   //lvgl_epd_ssd1677_init(SPI2_HOST); // or VSPI_HOST if you're using VSPI

   printf("display: We are done with the demo\n");
   //gpio_set_level(GPIO_DISPLAY_POWER_ON, 0);
   //esp_task_wdt_deinit();
   //epd_flush_queue = xQueueCreate(1, sizeof(epd_flush_req_t));
   //xTaskCreatePinnedToCore(epd_flush_task, "epd_flush_task", 4096, NULL, 5, NULL, 1); // chạy core 1

   //xTaskCreatePinnedToCore(lvgl_handle_task, "lvgl_handle_task", 1024 * 50, NULL, 2, NULL, 0);

   /*vTaskDelay(1000 / portTICK_PERIOD_MS);
   lv_obj_t *label = lv_label_create(lv_scr_act());
   lv_label_set_text(label, "Hello, LVGL!");
   lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

   vTaskDelay(1000 / portTICK_PERIOD_MS);
   lv_obj_t *rect = lv_obj_create(lv_scr_act());
   lv_obj_set_size(rect, 100, 100);
   lv_obj_center(rect);
   lv_obj_set_style_bg_color(rect, lv_color_black(), 0);*/
   vTaskDelay(1000 / portTICK_PERIOD_MS);
   printf("display: Black\n");
   //uint8_t color_p[EPD_HOR_RES * EPD_VER_RES / 8] = {};
   EPD_W21_Init_Fast(); //Fast refresh initialization.
   EPD_WhiteScreen_ALL_Fast(gImage_2); //To display one image using fast refresh.
   EPD_Dis_PartAll(DispImg2);
   EPD_DeepSleep();
   delay(2000); //Delay for 2s.
   vTaskDelay(5000 / portTICK_PERIOD_MS);
   /*

   printf("display: White\n");
   memset(color_p, 0xFF, BUF_PIXEL_COUNT / 8);
   EPD_WhiteScreen_ALL(color_p);*/

}