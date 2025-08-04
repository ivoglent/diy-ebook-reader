//
// Created by long.nguyenviet on 7/18/25.
//

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <gdeh0213b73.h>
#include <dke/depg1020bn.h>
#include <goodisplay/gdey042T81.h>

#define IS_COLOR_EPD 1

EpdSpi io;

Gdey042T81 display(io);

// Enable Power on Display
#define GPIO_DISPLAY_POWER_ON GPIO_NUM_38
// FONT used for title / message body - Only after display library
//Converting fonts with ümlauts: ./fontconvert *.ttf 18 32 252
//#include <Fonts/Vietnamese/Roboto20pt.h>
#include <Fonts/Vietnamese/Roboto_Regular10pt7b.h>
#include <stdint.h>

extern "C"
{
   void app_main();
}
void delay(uint32_t millis) { vTaskDelay(millis / portTICK_PERIOD_MS); }

void printUtf8String(const char* str) {
   uint32_t codepoint;
   while (*str) {
      uint8_t c = *str++;
      if ((c & 0x80) == 0) {
         codepoint = c;
      } else if ((c & 0xE0) == 0xC0) {
         codepoint = ((c & 0x1F) << 6) | (*str++ & 0x3F);
      } else if ((c & 0xF0) == 0xE0) {
         uint8_t c2 = *str++;
         uint8_t c3 = *str++;
         codepoint = ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
      } else {
         continue; // unsupported byte
      }
      display.write(codepoint); // Works if font includes this glyph
   }
   display.update();
}
// Thanks Konstantin!
void draw_content_lines(uint8_t rotation){
   // Sizes are calculated dividing the screen in 4 equal parts it may not be perfect for all models


   uint16_t foregroundColor = EPD_BLACK;
   // Make some rectangles showing the different colors or grays
   if (display.colors_supported>1) {
      printf("display.colors_supported:%d\n", display.colors_supported);
      foregroundColor = 0xF700; // RED
   }

   display.setFont(&Roboto_Regular10pt7b);
   display.setTextColor(foregroundColor);
   display.setCursor(4, 20);
   display.println("here are many variations of passages of Lorem Ipsum available, but the majority have suffered alteration in some form, by injected humour, or randomised words which don't look even slightly believable. If you are going to use a passage of Lorem Ipsum, you need to be sure there isn't anything embarrassing hidden in the middle of text. All the Lorem Ipsum generators on the Internet tend to repeat predefined chunks as necessary, making this the first true generator on the Internet. It uses a dictionary of over 200");
   //display.println("Cộng hoà xã hội chủ nghĩa Việt Nam");
   //display.setTextColor(foregroundColor);
   //display.println("Độc lập - tự do - hạnh phúc!");
   display.update();
   delay(1000);
}


void app_main(void)
{
   // This is just in case you power your epaper VCC with a GPIO
   gpio_set_direction(GPIO_DISPLAY_POWER_ON, GPIO_MODE_OUTPUT);
   gpio_set_level(GPIO_DISPLAY_POWER_ON, 1);
   //delay(100);
   printf("CalEPD version: %s\n", CALEPD_VERSION);
   display.init();
   display.fillScreen(EPD_WHITE);
   draw_content_lines(0);

   delay(2500);

   //display.fillScreen(EPD_WHITE);


   #ifndef IS_COLOR_EPD
      display.setMonoMode(true);
   #endif

   printf("EPD width: %d height: %d\n\n", display.width(), display.height());

   printf("display: We are done with the demo\n");
   gpio_set_level(GPIO_DISPLAY_POWER_ON, 0);
}