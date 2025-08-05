//
// Created by long.nguyenviet on 7/18/25.
//

#include <esp_log.h>
#include <nvs_flash.h>
#include <stdio.h>
#include <driver/gpio.h>
#include <driver/spi_common.h>
#include <hal/spi_types.h>

#include "components/GDEQ0426T82/EPD_W21.h"
#include "components/GDEQ0426T82/GUI/GUI_Paint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define IMG_INDEX_MAX   EPD_ARRAY
void __ESP32_SPI_Bus_Init(spi_host_device_t host_id);
uint8_t GUIImg2[IMG_INDEX_MAX]; //缓存界面
uint8_t *DispImg2; //一个bit存一个pix

// SPI bus
#define SPI_HOST_USING  SPI2_HOST //HSPI_HOST
#define SPI_PIN_MOSI    18
#define SPI_PIN_MISO    -1
#define SPI_PIN_SCLK    19


#define POWER_PIN  static_cast<gpio_num_t>(38)

#define TAG_MAIN "main"

void power_on()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << POWER_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    // 2. Set chân GPIO 38 lên HIGH
    gpio_set_level(POWER_PIN, 1);
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
    ESP_LOGI(TAG_MAIN, "nvs init done");

    power_on();

    //------------ init SPI bus
    __ESP32_SPI_Bus_Init(SPI_HOST_USING);


    vTaskDelay(pdMS_TO_TICKS(1000));

    DispImg2 = GUIImg2;

    //------------ 使用GUI, 并支持局部刷新
    GUI_Init_Img(GUIImg2, EPD_X_LEN, EPD_Y_LEN, Rotate0, ColorWhite); //需要交换W和H
    GUI_Clear_All(ColorWhite);
    //
    EPD_W21_Init_GUI();
    EPD_SetRAMValue_BaseMap(GUIImg2); //设置背景

    //GUI_Paint_DrawString_EN(300, 50, "Cộng hoà xã hội chủ nghĩa Việt Nam!", &Font16VN, ColorWhite, ColorBlack);
    GUI_Paint_DrawTextUTF8(20, 100, "Chúc mừng năm mới", &Font16VN, ColorWhite, ColorBlack);  // đen trắng
    GUI_Paint_DrawString_VN(200, 100, "Chúc mừng năm mới", &Font16VN, ColorWhite, ColorBlack);  // đen trắng
    //GUI_Paint_DrawString_EN(300, 50, "Hello world!!! ắ +", &Font16VN, ColorWhite, ColorBlack);
    GUI_Paint_DrawString_EN(300, 50, "Chúc mừng năm mới!!!", &Font16, ColorWhite, ColorBlack);
    GUI_Paint_DrawString_EN(100, 50, "Hello world!!!", &Font16, ColorWhite, ColorBlack);
    EPD_Dis_PartAll(DispImg2);

    EPD_DeepSleep();
}

// @brief: SPI bus init(SPI2_HOST)
//        SPI0/1为SPI Memory模式; SPI2为GP SPI
//
//        SPI bus init需要指定MOSI/MISO/SCLK,
//        而CS在各个SPI device init时指定
//
//        WP(WriteProtect)/HD(HolD)暂未指定
//
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

    //------------ init EPD
    // 初始化GPIO & 添加SPI device
    ePaper_InitHardware(SPI_HOST_USING);
    // 将EPD处理函数交给WebServer
    // handler_ePaperSetData = ePaper_SetImgData; //TODO:ePaper_SetImgData不再使用
    //handler_ePaperDisplay = ePaper_FullDispaly;
    //handler_ePaperResetIdx = ePaper_ResetImgIndex;
    ESP_LOGI(TAG_MAIN, ">> init ePaper");

    ePaper_GuiImg_Init();

    //ePaper_Test_1();
    //ePapre_Test_2_UiGenerate();


}
