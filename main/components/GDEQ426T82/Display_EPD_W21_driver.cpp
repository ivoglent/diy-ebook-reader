
#include "Display_EPD_W21_driver.h"

#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"


/************************
 * DEFINES
 ************************/
#define TAG_DRV             "EPD Driver"

/************************
 * VARIABLES
 ************************/
spi_device_handle_t SPI_Device_Handle;


//************************************************************************************************************
//************************************ low level function - start ********************************************

// SPI host
// #define SPI_HOST_USING    SPI2_HOST

// pins
// #define PIN_MOSI 3
// #define PIN_CLK  2
//


void __SPI_Each_Trans_Callback(spi_transaction_t *spi_trans)
{
    int dc = (int)spi_trans->user;
    gpio_set_level(PIN_DC, dc);
}

//==================================== GPIO初始化 & SPI device初始化
//
void __ESP32_GPIO_SPI_Init(spi_host_device_t host_id)
{
    esp_err_t ret;

    //------------ GPIO init ------------
    //D/C, RES
    gpio_config_t ioOutputCfg = {
        .pin_bit_mask = (1ULL<<PIN_DC) | (1ULL<<PIN_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&ioOutputCfg);
    //BUSY
    gpio_config_t ioIntputCfg = {
        .pin_bit_mask = (1ULL<<PIN_BUSY),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&ioIntputCfg);
    //set init IO
    gpio_set_level(PIN_CS, 1);

    //------------ SPI device init ------------
    // spi_device_handle_t spi;

    spi_device_interface_config_t spiDevInterCfg = {
        .mode= 0,  //TODO: 也许是其他模式
        .clock_speed_hz = 10*1000*1000,
        .spics_io_num = PIN_CS,
        .queue_size = 7,
        .pre_cb = __SPI_Each_Trans_Callback,
    };
    
    //attach device to spi bus
    // ret = spi_bus_add_device(host_id, &spiDevInterCfg, &spi);
    ret = spi_bus_add_device(host_id, &spiDevInterCfg, &SPI_Device_Handle);
    ESP_ERROR_CHECK(ret);
    // SPI_Device_Handle = spi; //保存SPI_Device_Handle到全局变量中

    //------------ EPD init ------------

}

void __SPI_Write_Cmd(spi_device_handle_t spi, const uint8_t cmd, bool keep_cs_active)
{
    esp_err_t ret;
    spi_transaction_t spiTrans;

    memset(&spiTrans, 0, sizeof(spiTrans));       //Zero out the transaction
    
    spiTrans.length = 1 * 8;                     //Command is 8 bits
    spiTrans.tx_buffer = &cmd;               //The data is the cmd itself
    spiTrans.user = (void*)0;                //D/C needs to be set to 0, for cmd
    if (keep_cs_active) {
      spiTrans.flags = SPI_TRANS_CS_KEEP_ACTIVE;   //Keep CS active after data transfer
    }

    ret=spi_device_polling_transmit(spi, &spiTrans);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

void __SPI_Write_Data(spi_device_handle_t spi, const uint8_t data)
{
    esp_err_t ret;
    spi_transaction_t spiTrans;

    memset(&spiTrans, 0, sizeof(spiTrans));       //Zero out the transaction
    spiTrans.length = 1 * 8;                 //Len is in bytes, transaction length is in bits.
    spiTrans.tx_buffer = &data;               //Data
    spiTrans.user = (void*)1;                //D/C needs to be set to 1, for data

    ret=spi_device_polling_transmit(spi, &spiTrans);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

//************************************ low level function - end **********************************************
//************************************************************************************************************



//************************************************************************************************************
//************************************ EPD function - start **********************************************

/************************
 * MACRO
 ************************/
#define EPD_W21_Get_BUSY            gpio_get_level(PIN_BUSY)
#define EPD_W21_Set_RST_0            gpio_set_level(PIN_RST, 0)
#define EPD_W21_Set_RST_1            gpio_set_level(PIN_RST, 1)

/************************
 * EPD FUNC
 ************************/


// Description: 
//      ESP32硬件初始化, IO和SPI
//
void EPD_HW_Init(spi_host_device_t host_id)
{
    __ESP32_GPIO_SPI_Init(host_id);
}

void __EPD_W21_Write_Cmd(uint8_t cmd)
{
    __SPI_Write_Cmd(SPI_Device_Handle, cmd, false);
}

void __EPD_W21_Write_Data(uint8_t data)
{
    __SPI_Write_Data(SPI_Device_Handle, data);
}

//Delay Functions
void delay_Xms(unsigned int Xms)
{
    vTaskDelay(Xms / portTICK_PERIOD_MS);
}

// Description:
//      阻塞读取BUSY pin状态
//
void __Epaper_ReadBusy(void)
{ 
    // ESP_LOGI(TAG_DRV, "try to read busy...");

    while(1)
    {     //=1 BUSY
        if(EPD_W21_Get_BUSY == 0) 
            break;
        else
            vTaskDelay(3); //防止一直等待触发WDT
    }

    // ESP_LOGI(TAG_DRV, "EPD busy rls...");
}

//////////////////////////////Display Update Function///////////////////////////////////////////////////////

//Full screen refresh update function
void __EPD_Update(void)
{   
    __EPD_W21_Write_Cmd(0x22); //Display Update Control
    __EPD_W21_Write_Data(0xF7);   
    __EPD_W21_Write_Cmd(0x20); //Activate Display Update Sequence

    __Epaper_ReadBusy();   
}

//Fast refresh 1 update function
void __EPD_Update_Fast(void)
{   
    __EPD_W21_Write_Cmd(0x22); //Display Update Control
    __EPD_W21_Write_Data(0xC7);   
    __EPD_W21_Write_Cmd(0x20); //Activate Display Update Sequence

    __Epaper_ReadBusy();   
}

//Partial refresh update function
void EPD_Part_Update(void)
{
    __EPD_W21_Write_Cmd(0x22); //Display Update Control
    __EPD_W21_Write_Data(0xFF);   
    __EPD_W21_Write_Cmd(0x20); //Activate Display Update Sequence

    __Epaper_ReadBusy();            
}

//Partial refresh write address and data
// input:
//   x_start, 图像开始的x坐标
//   y_start, 图像开始的y坐标
//   datas, 图像的数据
//   PART_COLUMN, 局部图像的尺寸, 再配合y_start计算y_end
//   PART_LINE, 局部图图像的尺寸, 再配合x_start计算x_end
//
void EPD_Dis_Part_RAM(unsigned int x_start,unsigned int y_start,
                      const unsigned char * datas,
                      unsigned int PART_COLUMN,unsigned int PART_LINE)
{
    unsigned int i;  
    unsigned int x_end,y_end;

    x_start=x_start-x_start%8; //x address start
    x_end=x_start+PART_LINE-1; //x address end
    y_start=y_start; //Y address start
    y_end=y_start+PART_COLUMN-1; //Y address end

    //Reset
    EPD_W21_Set_RST_0;  // Module reset   
    delay_Xms(10);//At least 10ms delay 
    EPD_W21_Set_RST_1;
    delay_Xms(10); //At least 10ms delay 

    __EPD_W21_Write_Cmd(0x18); 
    __EPD_W21_Write_Data(0x80);

    __EPD_W21_Write_Cmd(0x3C); //BorderWavefrom  
    __EPD_W21_Write_Data(0x80);    
    //    

    __EPD_W21_Write_Cmd(0x44);       // set RAM x address start/end
    __EPD_W21_Write_Data(x_start%256);  //x address start2 
    __EPD_W21_Write_Data(x_start/256); //x address start1 
    __EPD_W21_Write_Data(x_end%256);  //x address end2 
    __EPD_W21_Write_Data(x_end/256); //x address end1   
    __EPD_W21_Write_Cmd(0x45);    // set RAM y address start/end
    __EPD_W21_Write_Data(y_start%256);  //y address start2 
    __EPD_W21_Write_Data(y_start/256); //y address start1 
    __EPD_W21_Write_Data(y_end%256);  //y address end2 
    __EPD_W21_Write_Data(y_end/256); //y address end1   

    __EPD_W21_Write_Cmd(0x4E);        // set RAM x address count to 0;
    __EPD_W21_Write_Data(x_start%256);  //x address start2 
    __EPD_W21_Write_Data(x_start/256); //x address start1 
    __EPD_W21_Write_Cmd(0x4F);      // set RAM y address count to 0X127;    
    __EPD_W21_Write_Data(y_start%256);//y address start2
    __EPD_W21_Write_Data(y_start/256);//y address start1

    __EPD_W21_Write_Cmd(0x24);   //Write Black and White image to RAM
    for(i=0;i<PART_COLUMN*PART_LINE/8;i++)
    {                         
        __EPD_W21_Write_Data(datas[i]);
    } 
}

////////////////////////////////////E-paper demo//////////////////////////////////////////////////////////

// Description:
//      EPD执行reset, 清屏
//
void EPD_W21_Init(void)
{
    // reset
    EPD_W21_Set_RST_1;
    // ESP_LOGI(TAG_DRV, "RST 1");
    delay_Xms(50);
    EPD_W21_Set_RST_0;  // Module reset
    // ESP_LOGI(TAG_DRV, "RST 0");
    delay_Xms(50);//At least 10ms delay
    EPD_W21_Set_RST_1;
    // ESP_LOGI(TAG_DRV, "RST 1");
    delay_Xms(50); //At least 10ms delay

    __Epaper_ReadBusy();   
    __EPD_W21_Write_Cmd(0x12);  //SWRESET
    __Epaper_ReadBusy();   

    __EPD_W21_Write_Cmd(0x18);   
    __EPD_W21_Write_Data(0x80); 

    __EPD_W21_Write_Cmd(0x0C);
    __EPD_W21_Write_Data(0xAE);
    __EPD_W21_Write_Data(0xC7);
    __EPD_W21_Write_Data(0xC3);
    __EPD_W21_Write_Data(0xC0);
    __EPD_W21_Write_Data(0x80);

    __EPD_W21_Write_Cmd(0x01); //Driver output control      
    __EPD_W21_Write_Data((EPD_WIDTH-1)%256);    
    __EPD_W21_Write_Data((EPD_WIDTH-1)/256);
    __EPD_W21_Write_Data(0x02); 

    __EPD_W21_Write_Cmd(0x3C); //BorderWavefrom
    __EPD_W21_Write_Data(0x01);    

    __EPD_W21_Write_Cmd(0x11); //data entry mode       
    __EPD_W21_Write_Data(0x03);

    __EPD_W21_Write_Cmd(0x44); //set Ram-X address start/end position   
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data((EPD_HEIGHT-1)%256);   
    __EPD_W21_Write_Data((EPD_HEIGHT-1)/256);

    __EPD_W21_Write_Cmd(0x45); //set Ram-Y address start/end position          
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00); 
    __EPD_W21_Write_Data((EPD_WIDTH-1)%256);   
    __EPD_W21_Write_Data((EPD_WIDTH-1)/256);


    __EPD_W21_Write_Cmd(0x4E);   // set RAM x address count to 0;
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Cmd(0x4F);   // set RAM y address count to 0X199;    
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00);
    __Epaper_ReadBusy();

    ESP_LOGI(TAG_DRV, "EPD W21 Init done...");
}

void EPD_W21_Init_Modes(Init_Mode_t mode)
{
    // reset
    EPD_W21_Set_RST_1;
    delay_Xms(50);
    EPD_W21_Set_RST_0;  // Module reset
    delay_Xms(50);//At least 10ms delay
    EPD_W21_Set_RST_1;
    delay_Xms(50); //At least 10ms delay

    __Epaper_ReadBusy();   
    __EPD_W21_Write_Cmd(0x12);  //SWRESET
    __Epaper_ReadBusy();   

    __EPD_W21_Write_Cmd(0x18);   
    __EPD_W21_Write_Data(0x80); 

    __EPD_W21_Write_Cmd(0x0C);
    __EPD_W21_Write_Data(0xAE);
    __EPD_W21_Write_Data(0xC7);
    __EPD_W21_Write_Data(0xC3);
    __EPD_W21_Write_Data(0xC0);
    __EPD_W21_Write_Data(0x80);

    __EPD_W21_Write_Cmd(0x01); //Driver output control      
    __EPD_W21_Write_Data((EPD_WIDTH-1)%256);    
    __EPD_W21_Write_Data((EPD_WIDTH-1)/256);
    __EPD_W21_Write_Data(0x02); 

    __EPD_W21_Write_Cmd(0x3C); //BorderWavefrom
    __EPD_W21_Write_Data(0x01);    

    ///////// 模式分支
    if(mode == Init_Mode_X_0)
    {
        __EPD_W21_Write_Cmd(0x11); //data entry mode       
        __EPD_W21_Write_Data(0x00); //X decrease, Y decrease

        __EPD_W21_Write_Cmd(0x44); //set Ram-X address start/end position   
        __EPD_W21_Write_Data((EPD_HEIGHT-1)%256);   
        __EPD_W21_Write_Data((EPD_HEIGHT-1)/256);
        __EPD_W21_Write_Data(0x00);
        __EPD_W21_Write_Data(0x00);

        __EPD_W21_Write_Cmd(0x45); //set Ram-Y address start/end position          
        __EPD_W21_Write_Data((EPD_WIDTH-1)%256);   
        __EPD_W21_Write_Data((EPD_WIDTH-1)/256);
        __EPD_W21_Write_Data(0x00);
        __EPD_W21_Write_Data(0x00);
    }
    else if(mode == Init_Mode_X_1)
    {
        __EPD_W21_Write_Cmd(0x11); //data entry mode       
        __EPD_W21_Write_Data(0x01); //X increase, Y decrease

        __EPD_W21_Write_Cmd(0x44); //set Ram-X address start/end position   
        __EPD_W21_Write_Data(0x00);
        __EPD_W21_Write_Data(0x00);
        __EPD_W21_Write_Data((EPD_HEIGHT-1)%256);   
        __EPD_W21_Write_Data((EPD_HEIGHT-1)/256);

        __EPD_W21_Write_Cmd(0x45); //set Ram-Y address start/end position          
        __EPD_W21_Write_Data((EPD_WIDTH-1)%256);   
        __EPD_W21_Write_Data((EPD_WIDTH-1)/256);
        __EPD_W21_Write_Data(0x00);
        __EPD_W21_Write_Data(0x00);
    }
    else if(mode == Init_Mode_X_2)
    {
        __EPD_W21_Write_Cmd(0x11); //data entry mode       
        __EPD_W21_Write_Data(0x02); //X decrease, Y increase

        __EPD_W21_Write_Cmd(0x44); //set Ram-X address start/end position   
        __EPD_W21_Write_Data((EPD_HEIGHT-1)%256);   
        __EPD_W21_Write_Data((EPD_HEIGHT-1)/256);
        __EPD_W21_Write_Data(0x00);
        __EPD_W21_Write_Data(0x00);

        __EPD_W21_Write_Cmd(0x45); //set Ram-Y address start/end position
        __EPD_W21_Write_Data(0x00);
        __EPD_W21_Write_Data(0x00);     
        __EPD_W21_Write_Data((EPD_WIDTH-1)%256);   
        __EPD_W21_Write_Data((EPD_WIDTH-1)/256);
    }
    else //Init_Mode_X_3
    {
        __EPD_W21_Write_Cmd(0x11); //data entry mode       
        __EPD_W21_Write_Data(0x03); //X increas, Y increase

        __EPD_W21_Write_Cmd(0x44); //set Ram-X address start/end position   
        __EPD_W21_Write_Data(0x00);
        __EPD_W21_Write_Data(0x00);
        __EPD_W21_Write_Data((EPD_HEIGHT-1)%256);   
        __EPD_W21_Write_Data((EPD_HEIGHT-1)/256);

        __EPD_W21_Write_Cmd(0x45); //set Ram-Y address start/end position
        __EPD_W21_Write_Data(0x00);
        __EPD_W21_Write_Data(0x00);       
        __EPD_W21_Write_Data((EPD_WIDTH-1)%256);   
        __EPD_W21_Write_Data((EPD_WIDTH-1)/256);
    }

    ///////////
    // __EPD_W21_Write_Cmd(0x11); //data entry mode       
    // __EPD_W21_Write_Data(0x03);

    // __EPD_W21_Write_Cmd(0x44); //set Ram-X address start/end position   
    // __EPD_W21_Write_Data(0x00);
    // __EPD_W21_Write_Data(0x00);
    // __EPD_W21_Write_Data((EPD_HEIGHT-1)%256);   
    // __EPD_W21_Write_Data((EPD_HEIGHT-1)/256);

    // __EPD_W21_Write_Cmd(0x45); //set Ram-Y address start/end position          
    // __EPD_W21_Write_Data(0x00);
    // __EPD_W21_Write_Data(0x00); 
    // __EPD_W21_Write_Data((EPD_WIDTH-1)%256);   
    // __EPD_W21_Write_Data((EPD_WIDTH-1)/256);

    ///////////    
    // __EPD_W21_Write_Cmd(0x11); //data entry mode       
    // __EPD_W21_Write_Data(0x01); //默认0x03

    // __EPD_W21_Write_Cmd(0x44); //set Ram-X address start/end position   
    // __EPD_W21_Write_Data(0x00);
    // __EPD_W21_Write_Data(0x00);
    // __EPD_W21_Write_Data((EPD_HEIGHT-1)%256);   
    // __EPD_W21_Write_Data((EPD_HEIGHT-1)/256);

    // __EPD_W21_Write_Cmd(0x45); //set Ram-Y address start/end position          
    // __EPD_W21_Write_Data((EPD_WIDTH-1)%256);   
    // __EPD_W21_Write_Data((EPD_WIDTH-1)/256);
    // __EPD_W21_Write_Data(0x00);
    // __EPD_W21_Write_Data(0x00); 
    /////////

    __EPD_W21_Write_Cmd(0x4E);   // set RAM x address count to 0;
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Cmd(0x4F);   // set RAM y address count to 0X199;    
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00);

    __Epaper_ReadBusy();

    ESP_LOGI(TAG_DRV, "EPD W21 Init done...");
}

//Fast refresh 1 initialization
void EPD_W21_Init_Fast(void)
{
    EPD_W21_Set_RST_0;  // Module reset   
    delay_Xms(10);//At least 10ms delay 
    EPD_W21_Set_RST_1;
    delay_Xms(10); //At least 10ms delay 

    __Epaper_ReadBusy();   
    __EPD_W21_Write_Cmd(0x12);  //SWRESET
    __Epaper_ReadBusy();   

    __EPD_W21_Write_Cmd(0x18);   
    __EPD_W21_Write_Data(0x80); 

    __EPD_W21_Write_Cmd(0x0C);
    __EPD_W21_Write_Data(0xAE);
    __EPD_W21_Write_Data(0xC7);
    __EPD_W21_Write_Data(0xC3);
    __EPD_W21_Write_Data(0xC0);
    __EPD_W21_Write_Data(0x80);

    __EPD_W21_Write_Cmd(0x01); //Driver output control      
    __EPD_W21_Write_Data((EPD_WIDTH-1)%256);   
    __EPD_W21_Write_Data((EPD_WIDTH-1)/256);
    __EPD_W21_Write_Data(0x02);

    __EPD_W21_Write_Cmd(0x3C); //BorderWavefrom
    __EPD_W21_Write_Data(0x01);    

    __EPD_W21_Write_Cmd(0x11); //data entry mode       
    __EPD_W21_Write_Data(0x03);

    __EPD_W21_Write_Cmd(0x44); //set Ram-X address start/end position   
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data((EPD_HEIGHT-1)%256);    
    __EPD_W21_Write_Data((EPD_HEIGHT-1)/256);

    __EPD_W21_Write_Cmd(0x45); //set Ram-Y address start/end position          
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00); 
    __EPD_W21_Write_Data((EPD_WIDTH-1)%256);    
    __EPD_W21_Write_Data((EPD_WIDTH-1)/256);


    __EPD_W21_Write_Cmd(0x4E);   // set RAM x address count to 0;
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Cmd(0x4F);   // set RAM y address count to 0X199;    
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00);
    __Epaper_ReadBusy();

    //TEMP (1.5s)
    __EPD_W21_Write_Cmd(0x1A); 
    __EPD_W21_Write_Data(0x5A);

    __EPD_W21_Write_Cmd(0x22); 
    __EPD_W21_Write_Data(0x91);
    __EPD_W21_Write_Cmd(0x20);

    __Epaper_ReadBusy();  
}


//////////////////////////////Display Data Transfer Function////////////////////////////////////////////

// Description:
//      用datas填充整个界面, 最后执行普通刷新(full screen refresh)
//
void EPD_WhiteScreen_ALL(const unsigned char *datas)
{
    unsigned int i;

    __EPD_W21_Write_Cmd(0x24);   //write RAM for black(0)/white (1)
    for(i = 0; i < EPD_ARRAY; i++)
    {               
        __EPD_W21_Write_Data(datas[i]);
    }     
    __EPD_Update();     
}

// Description:
//      用datas填充整个界面, 最后执行快速刷新(fast refresh)
//
void EPD_WhiteScreen_ALL_Fast(const unsigned char *datas)
{
   unsigned int i;

  __EPD_W21_Write_Cmd(0x24);   //write RAM for black(0)/white (1)
   for(i=0;i<EPD_ARRAY;i++)
   {               
     __EPD_W21_Write_Data(datas[i]);
   }       
   __EPD_Update_Fast();     
}

// Description:
//      清屏显示
//
void EPD_WhiteScreen_White(void)
{
    unsigned int i;

    __EPD_W21_Write_Cmd(0x24);   //write RAM for black(0)/white (1)
    for(i = 0; i < EPD_ARRAY; i ++)
    {
        __EPD_W21_Write_Data(0xFF);  //8bit像素点
    }
    __EPD_Update();

    // ESP_LOGI(TAG_DRV, );
}

// //Display all black
// void EPD_WhiteScreen_Black(void)
// {
//  unsigned int i;
//  EPD_W21_WriteCMD(0x24);   //write RAM for black(0)/white (1)
//  for(i=0;i<EPD_ARRAY;i++)
//  {
//         EPD_W21_WriteDATA(0x00);
//     }
//     __EPD_Update();
// }

//Partial refresh of background display, this function is necessary, please do not delete it!!!
void EPD_SetRAMValue_BaseMap( const unsigned char * datas)
{
    unsigned int i;

    __EPD_W21_Write_Cmd(0x24);   //Write Black or White image to RAM
    for(i=0;i<EPD_ARRAY;i++)
    {               
        __EPD_W21_Write_Data(datas[i]);
    }
    __EPD_W21_Write_Cmd(0x26);   //Write Red or Other image to RAM
    for(i=0;i<EPD_ARRAY;i++)
    {               
        __EPD_W21_Write_Data(datas[i]);
    }

    __EPD_Update();           
}

// //Partial refresh display
// void EPD_Dis_Part(unsigned int x_start,unsigned int y_start,const unsigned char * datas,unsigned int PART_COLUMN,unsigned int PART_LINE)
// {
//     unsigned int i;  
//     unsigned int x_end,y_end;
    
//     x_start=x_start-x_start%8; //x address start
//     x_end=x_start+PART_LINE-1; //x address end
//     y_start=y_start; //Y address start
//     y_end=y_start+PART_COLUMN-1; //Y address end
    
// //Reset
//     EPD_W21_RST_0;  // Module reset   
//     delay_Xms(10);//At least 10ms delay 
//     EPD_W21_RST_1;
//     delay_Xms(10); //At least 10ms delay 

//     EPD_W21_WriteCMD(0x18); 
//     EPD_W21_WriteDATA(0x80);
    
//     EPD_W21_WriteCMD(0x3C); //BorderWavefrom  
//     EPD_W21_WriteDATA(0x80);    
// //    
    
//     EPD_W21_WriteCMD(0x44);       // set RAM x address start/end
//     EPD_W21_WriteDATA(x_start%256);  //x address start2 
//     EPD_W21_WriteDATA(x_start/256); //x address start1 
//     EPD_W21_WriteDATA(x_end%256);  //x address end2 
//     EPD_W21_WriteDATA(x_end/256); //x address end1   
//     EPD_W21_WriteCMD(0x45);    // set RAM y address start/end
//     EPD_W21_WriteDATA(y_start%256);  //y address start2 
//     EPD_W21_WriteDATA(y_start/256); //y address start1 
//     EPD_W21_WriteDATA(y_end%256);  //y address end2 
//     EPD_W21_WriteDATA(y_end/256); //y address end1   

//     EPD_W21_WriteCMD(0x4E);        // set RAM x address count to 0;
//     EPD_W21_WriteDATA(x_start%256);  //x address start2 
//     EPD_W21_WriteDATA(x_start/256); //x address start1 
//     EPD_W21_WriteCMD(0x4F);      // set RAM y address count to 0X127;    
//     EPD_W21_WriteDATA(y_start%256);//y address start2
//     EPD_W21_WriteDATA(y_start/256);//y address start1
    
    
//      EPD_W21_WriteCMD(0x24);   //Write Black and White image to RAM
//    for(i=0;i<PART_COLUMN*PART_LINE/8;i++)
//    {                         
//      EPD_W21_WriteDATA(datas[i]);
//    } 
//      EPD_Part_Update();

// }

//Full screen partial refresh display
void EPD_Dis_PartAll(const unsigned char * datas)
{
    unsigned int i;  
    unsigned int PART_COLUMN, PART_LINE;
    PART_COLUMN=EPD_HEIGHT,PART_LINE=EPD_WIDTH;

    //Reset
    EPD_W21_Set_RST_0;// Module reset   
    delay_Xms(10);//At least 10ms delay 
    EPD_W21_Set_RST_1;
    delay_Xms(10); //At least 10ms delay 

    __EPD_W21_Write_Cmd(0x18); 
    __EPD_W21_Write_Data(0x80);

    __EPD_W21_Write_Cmd(0x3C); //BorderWavefrom  
    __EPD_W21_Write_Data(0x80);    
    //    

    __EPD_W21_Write_Cmd(0x24);   //Write Black and White image to RAM
    for(i=0;i<PART_COLUMN*PART_LINE/8;i++)
    {                         
        __EPD_W21_Write_Data(datas[i]);
    }

    EPD_Part_Update();
}

// Descripiton:
//      进入DeepSleep模式
//
void EPD_DeepSleep(void)
{      
    __EPD_W21_Write_Cmd(0x10); //Enter deep sleep
    __EPD_W21_Write_Data(0x01);

    delay_Xms(100);
}

//Clock display
void EPD_Dis_Part_Time(unsigned int x_startA,unsigned int y_startA,const unsigned char * datasA,
                       unsigned int x_startB,unsigned int y_startB,const unsigned char * datasB,
                       unsigned int x_startC,unsigned int y_startC,const unsigned char * datasC,
                       unsigned int x_startD,unsigned int y_startD,const unsigned char * datasD,
                       unsigned int x_startE,unsigned int y_startE,const unsigned char * datasE,
                       unsigned int PART_COLUMN,unsigned int PART_LINE)
{
    EPD_Dis_Part_RAM(x_startA,y_startA,datasA,PART_COLUMN,PART_LINE);
    EPD_Dis_Part_RAM(x_startB,y_startB,datasB,PART_COLUMN,PART_LINE);
    EPD_Dis_Part_RAM(x_startC,y_startC,datasC,PART_COLUMN,PART_LINE);
    EPD_Dis_Part_RAM(x_startD,y_startD,datasD,PART_COLUMN,PART_LINE);
    EPD_Dis_Part_RAM(x_startE,y_startE,datasE,PART_COLUMN,PART_LINE);

    EPD_Part_Update();
}                                                 




////////////////////////////////Other newly added functions////////////////////////////////////////////
//Display rotation 180 degrees initialization
void EPD_W21_Init_180(void)
{
    EPD_W21_Set_RST_0;// Module reset   
    delay_Xms(10);//At least 10ms delay 
    EPD_W21_Set_RST_1;
    delay_Xms(10); //At least 10ms delay 

    __Epaper_ReadBusy();
    __EPD_W21_Write_Cmd(0x12);  //SWRESET
    __Epaper_ReadBusy();   

    __EPD_W21_Write_Cmd(0x18);   
    __EPD_W21_Write_Data(0x80); 

    __EPD_W21_Write_Cmd(0x0C);
    __EPD_W21_Write_Data(0xAE);
    __EPD_W21_Write_Data(0xC7);
    __EPD_W21_Write_Data(0xC3);
    __EPD_W21_Write_Data(0xC0);
    __EPD_W21_Write_Data(0x80);

    __EPD_W21_Write_Cmd(0x01); //Driver output control      
    __EPD_W21_Write_Data((EPD_WIDTH-1)%256);    
    __EPD_W21_Write_Data((EPD_WIDTH-1)/256);
    __EPD_W21_Write_Data(0x02);

    __EPD_W21_Write_Cmd(0x3C); //BorderWavefrom
    __EPD_W21_Write_Data(0x01);    

    __EPD_W21_Write_Cmd(0x11); //data entry mode       
    __EPD_W21_Write_Data(0x00); //180

    __EPD_W21_Write_Cmd(0x44); //set Ram-X address start/end position   

    __EPD_W21_Write_Data((EPD_HEIGHT-1)%256);    
    __EPD_W21_Write_Data((EPD_HEIGHT-1)/256);
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00);

    __EPD_W21_Write_Cmd(0x45); //set Ram-Y address start/end position          
    __EPD_W21_Write_Data((EPD_WIDTH-1)%256);    
    __EPD_W21_Write_Data((EPD_WIDTH-1)/256);
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00); 


    __EPD_W21_Write_Cmd(0x4E);   // set RAM x address count to 0;
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Cmd(0x4F);   // set RAM y address count to 0X199;    
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00);

    __Epaper_ReadBusy();
}

//=========================== GUI initialization
// description:
//      GUI专用EPD init, 使用了DataEntryMode=0x03(X increase; Y increase)
//
void EPD_W21_Init_GUI(void)
{
    EPD_W21_Set_RST_0;  // Module reset   
    delay_Xms(10);//At least 10ms delay 
    EPD_W21_Set_RST_1;
    delay_Xms(10); //At least 10ms delay 

    __Epaper_ReadBusy();   
    __EPD_W21_Write_Cmd(0x12);  //SWRESET
    __Epaper_ReadBusy();   

    __EPD_W21_Write_Cmd(0x18); //Temperature Sensor Control - 选择温度传感器
    __EPD_W21_Write_Data(0x80); //0x80=internal temperature sensor

    __EPD_W21_Write_Cmd(0x0C); //Booster Soft-start Control - 控制booster浪涌电流(inrush current)
    __EPD_W21_Write_Data(0xAE);
    __EPD_W21_Write_Data(0xC7);
    __EPD_W21_Write_Data(0xC3);
    __EPD_W21_Write_Data(0xC0);
    __EPD_W21_Write_Data(0x80); //0x40=level1; 0x80=level2

    __EPD_W21_Write_Cmd(0x01); //Driver output control      
    __EPD_W21_Write_Data((EPD_WIDTH-1)%256); //这里是低8bit   一共10bit, MUX Gate lines数量=(480-1)
    __EPD_W21_Write_Data((EPD_WIDTH-1)/256); //这里是高2bit
    __EPD_W21_Write_Data(0x02); //Gate扫描的顺序和方向, 扫描从G0到G480, 且有特定顺序

    __EPD_W21_Write_Cmd(0x3C); //Border Wavefrom Control - 为VBD选择board waveform
    __EPD_W21_Write_Data(0x01); // VBD设为GS Transition | VBD level设为VSS | VBD的GS Transition使用LUT2

    // __EPD_W21_Write_Cmd(0x11); //data entry mode - 定义数据进入顺序
    // __EPD_W21_Write_Data(0x02); //mirror   0x02=地址计数器在X方向更新 | Y increment, X decremnt
    // //
    // __EPD_W21_Write_Cmd(0x44); //set RAM-X address start/end position - 设置RAM中X方向的起始位置, 最大0x3BF=>960
    // __EPD_W21_Write_Data((EPD_HEIGHT-1)%256); //XStart低8bit
    // __EPD_W21_Write_Data((EPD_HEIGHT-1)/256); //XStart高2bit
    // __EPD_W21_Write_Data(0x00); //XEnd低8bit
    // __EPD_W21_Write_Data(0x00); //XEnd高2bit
    // //
    // __EPD_W21_Write_Cmd(0x45); //set RAM-Y address start/end position - 设置RAM中Y方向的起始位置, 最大0x2A7=>680
    // __EPD_W21_Write_Data(0x00); //YStart低8bit
    // __EPD_W21_Write_Data(0x00); //YStart高2bit 
    // __EPD_W21_Write_Data((EPD_WIDTH-1)%256); //YEnd低8bit
    // __EPD_W21_Write_Data((EPD_WIDTH-1)/256); //YEnd低8bit
    //------------ test
    __EPD_W21_Write_Cmd(0x11); //data entry mode       
    __EPD_W21_Write_Data(0x03);
    //
    __EPD_W21_Write_Cmd(0x44); //set Ram-X address start/end position   
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data((EPD_HEIGHT-1)%256);   
    __EPD_W21_Write_Data((EPD_HEIGHT-1)/256);
    //
    __EPD_W21_Write_Cmd(0x45); //set Ram-Y address start/end position          
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00); 
    __EPD_W21_Write_Data((EPD_WIDTH-1)%256);   
    __EPD_W21_Write_Data((EPD_WIDTH-1)/256);

    __EPD_W21_Write_Cmd(0x4E);   // set RAM x address count to 0;
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00);
    //
    __EPD_W21_Write_Cmd(0x4F);   // set RAM y address count to 0X199;
    __EPD_W21_Write_Data(0x00);
    __EPD_W21_Write_Data(0x00);

    __Epaper_ReadBusy();
}

//=========================== GUI display
// description:
//      显示img
//
void EPD_Display_GUI(unsigned char *img)
{
    uint16_t width, height,i,j;

    width = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
    height = EPD_HEIGHT;

    __EPD_W21_Write_Cmd(0x24); //Write RAM
    for ( j = 0; j < height; j++) //按列
    {
            for ( i = 0; i < width; i++) //按行 
            {
                 __EPD_W21_Write_Data(img[i + j * width]);
            }
    }
    __EPD_Update();
                
}

//*************************************** EPD function - end *************************************************
//************************************************************************************************************



