#ifndef _DISPLAY_EPD_W21_DRIVER_H_
#define _DISPLAY_EPD_W21_DRIVER_H_


#include "driver/spi_master.h"


#define EPD_WIDTH   480
#define EPD_Y_LEN   EPD_WIDTH
#define EPD_HEIGHT  800
#define EPD_X_LEN   EPD_HEIGHT
//
#define EPD_ARRAY   EPD_WIDTH * EPD_HEIGHT / 8 //创建了长度为480*800/8的char数组, 每个元素有8bit, 每个bit表示一个像素

#define PIN_CS   GPIO_NUM_11
//
#define PIN_DC   GPIO_NUM_12
#define PIN_RST    GPIO_NUM_5
#define PIN_BUSY  GPIO_NUM_10


typedef enum {
    //X-mode
    Init_Mode_X_0 = 0, //X decrease, Y decrease
    Init_Mode_X_1, //X increase, Y decrease
    Init_Mode_X_2, //X decrease, Y increase
    Init_Mode_X_3, //X increase, Y increase
    //Y-mode
    Init_Mode_Y_0,
    Init_Mode_Y_1,
    Init_Mode_Y_2,
    Init_Mode_Y_3,
} Init_Mode_t;


// //Full screen refresh display
// void EPD_WhiteScreen_Black(void);
// //Partial refresh display 
// //Fast refresh display  
// //GUI display
// void EPD_HW_Init_GUI(void);					 
// void EPD_Display(unsigned char *Image); 

// void EPD_HW_Init_180(void);	
// void EPD_HW_Init(void); 
// void EPD_HW_Init_Fast(void);
// void EPD_WhiteScreen_ALL(const unsigned char *datas);
// void EPD_WhiteScreen_White(void);
// void EPD_WhiteScreen_ALL_Fast(const unsigned char *datas); 
// void EPD_DeepSleep(void);
// void EPD_SetRAMValue_BaseMap(const unsigned char * datas);
// void EPD_Dis_Part_Time(unsigned int x_startA,unsigned int y_startA,const unsigned char * datasA,
// 						unsigned int x_startB,unsigned int y_startB,const unsigned char * datasB,
// 						unsigned int x_startC,unsigned int y_startC,const unsigned char * datasC,
// 						unsigned int x_startD,unsigned int y_startD,const unsigned char * datasD,
// 						unsigned int x_startE,unsigned int y_startE,const unsigned char * datasE,
// 						unsigned int PART_COLUMN,unsigned int PART_LINE);	
// void EPD_Dis_PartAll(const unsigned char * datas);
// void EPD_Dis_Part(unsigned int x_start,unsigned int y_start,const unsigned char * datas,unsigned int PART_COLUMN,unsigned int PART_LINE);								 


// Hardware Init
void EPD_HW_Init(spi_host_device_t host_id);

// Others
void EPD_W21_Init_Modes(Init_Mode_t mode);
// Common
void EPD_W21_Init();
void EPD_W21_Init_180(void);
void EPD_DeepSleep(void);
// Full screen refresh display
void EPD_WhiteScreen_White(void);
void EPD_WhiteScreen_ALL(const unsigned char *datas);
// Partial refresh display
void EPD_SetRAMValue_BaseMap( const unsigned char * datas);  //设置背景, 用于局部刷新
void EPD_Dis_Part_RAM(unsigned int x_start,unsigned int y_start,
                      const unsigned char * datas,
                      unsigned int PART_COLUMN,unsigned int PART_LINE); //更新局部图像
void EPD_Part_Update(void); //刷新屏幕
void EPD_Dis_Part_Time(unsigned int x_startA,unsigned int y_startA,const unsigned char * datasA,
                        unsigned int x_startB,unsigned int y_startB,const unsigned char * datasB,
                        unsigned int x_startC,unsigned int y_startC,const unsigned char * datasC,
                        unsigned int x_startD,unsigned int y_startD,const unsigned char * datasD,
                        unsigned int x_startE,unsigned int y_startE,const unsigned char * datasE,
                        unsigned int PART_COLUMN,unsigned int PART_LINE); //更新图像 并 刷新屏幕
void EPD_Dis_PartAll(const unsigned char * datas); //通过局部刷新更新整个屏幕
// Fast refresh display
void EPD_W21_Init_Fast(void);
void EPD_WhiteScreen_ALL_Fast(const unsigned char *datas);
// GUI display
void EPD_W21_Init_GUI(void);
void EPD_Display_GUI(unsigned char *img);


#endif



