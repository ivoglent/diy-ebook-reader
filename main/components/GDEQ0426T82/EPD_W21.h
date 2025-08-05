#ifndef __EPD_W21_H__
#define __EPD_W21_H__


#include "driver/spi_master.h"
//
// #include "../main.h"
#include "EPD/Display_EPD_W21_driver.h"


/************************************************************************
 * DEFINES
 ************************************************************************/
#define EPD_W21_WIDTH       EPD_WIDTH
#define EPD_W21_HEIGHT      EPD_HEIGHT


/************************************************************************
 * TYPES
 ************************************************************************/
typedef enum {
    Img_Type_Photo = 0,
    Img_Type_Gui
}Img_Type_t;


/************************************************************************
 * DECLARATION
 ************************************************************************/
void ePaper_InitHardware(spi_host_device_t host_id);
void ePaper_FullDispaly();
// esp_err_t ePaper_SetImgData(uint8_t imgVal);
esp_err_t ePaper_SetImgDataByChar(char *src, uint16_t len, Img_Type_t img_type);
void ePaper_ResetImgIndex();
int8_t ePaper_CheckImgIndex();
//
void ePaper_GuiImg_Init(void);
uint8_t ePaper_GuiImg_GetVal(uint16_t index);
void ePaper_GuiImg_UpdateSelect(uint8_t idx, bool clear);
void ePaper_GuiImg_Display();

void ePaper_Test_1(void);
void ePapre_Test_2_UiGenerate();







#endif

