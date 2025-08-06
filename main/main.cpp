//
// Created by long.nguyenviet on 7/18/25.
//

#include <esp_lcd_types.h>
#include <esp_log.h>
#include <stdio.h>
#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_task_wdt.h>
#include "lv_conf.h"
#define LV_CONF_INCLUDE_SIMPLE
#include "lvgl.h"

#include "components/GDEQ426T82/Ap_29demo.h"
#include "components/GDEQ426T82/Display_EPD_W21_driver.h"
#include "nvs_flash.h"
#define LV_COLOR_DEPTH     1
#define IS_COLOR_EPD 1

// Enable Power on Display
#define GPIO_DISPLAY_POWER_ON GPIO_NUM_38
#define BUF_PIXEL_COUNT (EPD_HOR_RES * EPD_VER_RES)


#define SPI_HOST_USING  SPI2_HOST //HSPI_HOST
#define SPI_PIN_MOSI    18
#define SPI_PIN_MISO    -1
#define SPI_PIN_SCLK    19

#define EPD_HOR_RES EPD_WIDTH
#define EPD_VER_RES EPD_HEIGHT

#if LV_COLOR_DEPTH != 1
#error "LV_COLOR_DEPTH must be 1 for monochrome EPD"
#endif

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


static QueueHandle_t epd_flush_queue;

typedef struct {
   lv_area_t area;
   uint8_t *buf;
   lv_disp_drv_t *disp_drv;
} epd_flush_req_t;

static lv_disp_draw_buf_t draw_buf;
//static uint8_t *lv_buf1 = nullptr;

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
void my_lvgl_flush_cb2(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
   // For simplicity, flush the whole screen (monochrome 1bpp)
   // color_p is assumed to be a full-screen 1-bit buffer
   ESP_LOGI("FLUSH", "Area: x(%d-%d), y(%d-%d), ptr: %p", area->x1, area->x2, area->y1, area->y2, color_p);
   ESP_LOG_BUFFER_HEXDUMP("LVGL_BUF", color_p, 16, ESP_LOG_INFO);
   //EPD_WhiteScreen_ALL(reinterpret_cast<uint8_t*>(color_p));
   //lv_disp_flush_ready(disp_drv);
   //return;
   epd_flush_req_t req;
   req.area = *area;
   req.disp_drv = disp_drv;
   // Allocate a buffer copy
   const size_t len = BUF_PIXEL_COUNT / 8;
   req.buf = static_cast<uint8_t*>(heap_caps_malloc(len, MALLOC_CAP_SPIRAM));
   if (req.buf == NULL)
   {
      ESP_LOGE("FLUSH", "Failed to allocate buffer");
   } else
   {
      memcpy(req.buf, color_p, len);
      xQueueSend(epd_flush_queue, &req, portMAX_DELAY);
   }

}

void my_lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
   if ((area->x1 != 0) || (area->y1 != 0) ||
       (area->x2 != EPD_WIDTH - 1) || (area->y2 != EPD_HEIGHT - 1))
   {
      ESP_LOGW("FLUSH", "Only full screen update supported");
      lv_disp_flush_ready(disp_drv);
      return;
   }

   uint8_t *converted_buf = (uint8_t *)heap_caps_malloc(EPD_WIDTH * EPD_HEIGHT / 8, MALLOC_CAP_DMA);
   if (!converted_buf) {
      ESP_LOGE("FLUSH", "Out of memory");
      lv_disp_flush_ready(disp_drv);
      return;
   }

   memset(converted_buf, 0xFF, EPD_WIDTH * EPD_HEIGHT / 8);

   for (int y = 0; y < EPD_HEIGHT; y++) {
      for (int x = 0; x < EPD_WIDTH; x++) {
         int pixel_index = y * EPD_WIDTH + x;
         int byte_index = (y * (EPD_WIDTH / 8)) + (x / 8);
         int bit_index = 7 - (x % 8);  // MSB first in each byte

         if (color_p[pixel_index].full) {
            // WHITE = 1 → set bit
            converted_buf[byte_index] |= (1 << bit_index);
         } else {
            // BLACK = 0 → clear bit
            converted_buf[byte_index] &= ~(1 << bit_index);
         }
         //ESP_LOGI("PIXEL", "pixel[%d][%d] = %d", x, y, color_p[pixel_index].full);

      }
   }

   EPD_WhiteScreen_ALL(converted_buf);
   free(converted_buf);
   lv_disp_flush_ready(disp_drv);
}


void lvgl_epd_ssd1677_init(spi_host_device_t spi_host)
{
   lv_init();
   const size_t buffer_pixels = EPD_HOR_RES * EPD_VER_RES;

   auto *buf1 = static_cast<lv_color_t*>(heap_caps_malloc(buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM));
   assert(buf1);
   auto *buf2 = static_cast<lv_color_t*>(heap_caps_malloc(buffer_pixels * sizeof(lv_color_t), MALLOC_CAP_SPIRAM));
   assert(buf2);
   lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buffer_pixels);
   static lv_disp_drv_t disp_drv;
   lv_disp_drv_init(&disp_drv);
   disp_drv.hor_res = EPD_HOR_RES;
   disp_drv.ver_res = EPD_VER_RES;
   disp_drv.flush_cb = my_lvgl_flush_cb;
   disp_drv.draw_buf = &draw_buf;

   lv_disp_drv_register(&disp_drv);
   ESP_LOGI("main", "LVGL EPD SSD1677 display initialized");
}

void lvgl_handle_task(void *pvParameters)
{
   ESP_LOGI("main", "Drawing example!!!");
   /*lv_style_t my_text_style;
   lv_style_init(&my_text_style);
   lv_style_set_text_font(&my_text_style, &lv_font_montserrat_16); // Using a built-in font


   lv_obj_t *label = lv_label_create(lv_scr_act());
   lv_label_set_text(label, "AAA");
   lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
   lv_obj_add_style(label, &my_text_style, 0);*/
   lv_obj_t *rect = lv_obj_create(lv_scr_act());
   lv_obj_set_size(rect, 200, 200);
   lv_obj_set_style_bg_color(rect, lv_color_black(), 0);
   lv_obj_set_style_bg_opa(rect, LV_OPA_COVER, 0);
   lv_obj_center(rect);
   // Delay a bit and then force flush
   vTaskDelay(pdMS_TO_TICKS(1000));
   lv_refr_now(NULL);

   while (1)
   {
      lv_timer_handler();
      vTaskDelay(pdMS_TO_TICKS(10));
   }
   vTaskDelete(NULL);
}

void spi_display_init(spi_host_device_t host_id)
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

   EPD_HW_Init(host_id);
   vTaskDelay(pdMS_TO_TICKS(100));
   EPD_W21_Init_Modes(Init_Mode_X_3);

}

extern "C"
void app_main(void)
{
   esp_err_t ret = nvs_flash_init();
   if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
   {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
   }
   ESP_ERROR_CHECK(ret);
   ESP_LOGI("main", "nvs init done");
   power_on();
   lvgl_epd_ssd1677_init(SPI_HOST_USING);
   spi_display_init(SPI_HOST_USING);
   vTaskDelay(pdMS_TO_TICKS(1000));
   ESP_LOGI("main", "Creating tasks...");
   epd_flush_queue = xQueueCreate(1, sizeof(epd_flush_req_t));
   xTaskCreatePinnedToCore(epd_flush_task, "epd_flush_task", 4096, NULL, 5, NULL, 1); // chạy core 1
   xTaskCreatePinnedToCore(lvgl_handle_task, "lvgl_handle_task", 1024 * 50, NULL, 2, NULL, 0);
   vTaskDelay(pdMS_TO_TICKS(1000));
   /*//Test
   EPD_WhiteScreen_White();
   ESP_LOGI("display_task", "Displaying image!");
   EPD_WhiteScreen_ALL(gImage_1); //To Display one image using full screen refresh.
   ESP_LOGI("display_task", "Enter deep sleep!");
   EPD_DeepSleep();
   */

   ESP_LOGI("main", "Done!");
}