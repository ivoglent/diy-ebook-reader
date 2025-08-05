/****************** ePapre high level API
 * 分别将接收图像缓存在PhotoImg, 将界面缓存在GUIImg中,
 * 固定使用DispImg进行显示, 要显示前将DispImg指向PhotoImg或GUIImg
 * 
 * 
 *  
 */

/************************************************************************
 * INCLUDE
 ************************************************************************/
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
//
#include "esp_log.h"
#include "esp_err.h"
//
// #include "driver/spi_master.h"
//
//
#include "EPD/Ap_29demo.h"
// #include "EPD/Display_EPD_W21_driver.h"

#include "EPD_W21.h"

#include "GUI/GUI_Paint.h"


/************************************************************************
 * DEFINES
 ************************************************************************/
#define TAG_EPD         "ePaper"
//
#define IMG_INDEX_MAX   EPD_ARRAY //48000=480*800/8
//
#define GUI_POSI_FILE_START         120
#define GUI_POSI_FILE_STEP          80
#define GUI_POSI_FILE_SELECT_Y      70
#define GUI_POSI_FILE_SELECT_R      20
#define GUI_POSI_FILE_NAME_Y        110


/************************************************************************
 * VARIABLE
 ************************************************************************/
volatile uint16_t ImgIndex = 0;
uint8_t PhotoImg[IMG_INDEX_MAX]; //缓存照片
uint8_t GUIImg[IMG_INDEX_MAX]; //缓存界面
uint8_t *DispImg; //一个bit存一个pix



/************************************************************************
 * FUNCTIONS
 ************************************************************************/
void ePaper_InitHardware(spi_host_device_t host_id)
{
    EPD_HW_Init(host_id);
    ESP_LOGI(TAG_EPD, "init ESP32 IO & SPI for EPD...");
}

//================== 初始化EPD, 并填充图像
// input:
//
void ePaper_FullDispaly()
{
    //TODO: DispImg指向PhotoImg
    DispImg = PhotoImg;

    // EPD_W21_Init(); //Full screen refresh initialization.
    // EPD_W21_Init_180();
    EPD_W21_Init_Modes(Init_Mode_X_3);
    EPD_WhiteScreen_ALL(DispImg); //To Display one image using full screen refresh.
    EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
    ESP_LOGI(TAG_EPD, ">>> full display..., ImgIndex=%d", ImgIndex);
    // vTaskDelay(2000 / portTICK_PERIOD_MS); //Delay for 2s.
}


//================== 设置页面数据DispImg[], 将使用1个uint8_t //TODO: 待删除
// input:
//      imgVal, 每次传入一个uint8_t, 表示8bit像素
//
// esp_err_t ePaper_SetImgData(uint8_t imgVal)
// {
//     // ESP_LOGI(TAG_EPD, "ImgIndex=%d here", ImgIndex);

//     // if(ImgIndex == (IMG_INDEX_MAX - 0))
//     // {
//     //     ESP_LOGI(TAG_EPD, "!!! ImgIndex is overflow !!!");
//     //     return ESP_FAIL;
//     // }
        
//     // // 倒叙显示
//     // DispImg[ImgIndex++] = imgVal;
//     // // 正序显示
//     // // DispImg[IMG_INDEX_MAX - (++ImgIndex)] = imgVal;
    

//     return ESP_OK;
// }

//================== 设置页面数据PhotoImg/GUIImg, 将使用2个char
// input:
//      将使用2个char的按特定方式拼接出一个uint8_t, 表示8bit像素
//
esp_err_t ePaper_SetImgDataByChar(char *src, uint16_t len, 
                                  Img_Type_t img_type)
{
    if(img_type == Img_Type_Photo)
        DispImg = PhotoImg; //用于读取文件填充Photo
    else
        DispImg = GUIImg; //用于读取文件填充Gui
    

    //检查ImgIndex
    if((ImgIndex + len / 2) > (IMG_INDEX_MAX - 0)) //每2个src[]拼出1个PhotoImg, 因此len/2是ImgIndex的增量
    {
        ESP_LOGI(TAG_EPD, "!!! ImgIndex is overflow !!!");
        return ESP_FAIL;
    }

    uint16_t current_pos = 0;
    while (current_pos < len)
    {
        //检查src中数据
        if((src[current_pos] < 'a') || (src[current_pos] > 'p') || //低4bit
        (src[current_pos + 1] < 'a') || (src[current_pos + 1] > 'p')) //高4bit
        {
            ESP_LOGI(TAG_EPD, "!!! src data is wrong !!!");
            return ESP_FAIL;
        }
        //拼接8bit图像数据 - 取两个字符(每个字符存4bit数据), 拼接出对应8bit数据
        uint8_t imgVal = ((int16_t)src[current_pos]-'a') | (((int16_t)src[current_pos + 1]-'a') << 4);
        //设置图像数据
        DispImg[ImgIndex++] = imgVal; // 倒叙显示
        // PhotoImg[IMG_INDEX_MAX - (++ImgIndex)] = imgVal; // 正序显示
        //向后迭代序号
        current_pos += 2; //src[]中每个元素存有4bit信息, 每次取2个元素
    }
    
    return ESP_OK;
}

//=========================== 重置PhotoImg的序号
// description:
//
void ePaper_ResetImgIndex()
{
    ESP_LOGI(TAG_EPD, "ImgIndex is %d, reset to 0", ImgIndex);

    //重置idx
    ImgIndex = 0;
}

//=========================== 检查PhotoImg的序号
// description:
//      正常接收Photo后, 应有ImgIndex=IMG_INDEX_MAX
// 
int8_t ePaper_CheckImgIndex()
{
    if(ImgIndex == IMG_INDEX_MAX)
        return 0;
    else
        return -1;
}


//=========================== GUI的快刷初始化
// description:
//
void ePaper_GuiImg_Init(void)
{
    //------------ 使用GUI, 并支持局部刷新
    GUI_Init_Img(GUIImg, EPD_X_LEN, EPD_Y_LEN, Rotate0, ColorWhite); //需要交换W和H
    GUI_Clear_All(ColorWhite);
    //
    EPD_W21_Init_GUI();
    EPD_SetRAMValue_BaseMap(GUIImg); //设置背景
}


//=========================== 使用 选择点 更新GUI
// description:
//
void ePaper_GuiImg_UpdateSelect(uint8_t idx, bool clear)
{
    Color_t color;

    if(clear)
        color = ColorWhite;
    else
        color = ColorBlack;

    //选择点
    GUI_Paint_DrawCircle(GUI_POSI_FILE_START + idx * GUI_POSI_FILE_STEP, GUI_POSI_FILE_SELECT_Y, 
                         GUI_POSI_FILE_SELECT_R, 
                         CircleStyle_Full, color, DotPixel_3x3);
}

//=========================== 返回GUIImg中指定位置数据
// description:
//      返回一个GUIImg数据
//      这个函数将在main.c中用于存储GUIImg到文件
//
uint8_t ePaper_GuiImg_GetVal(uint16_t index)
{
    return GUIImg[index];
}

//=========================== 用GUIImg刷新界面
// description:
//
void ePaper_GuiImg_Display()
{
    DispImg = GUIImg;

    EPD_Dis_PartAll(DispImg);
    EPD_DeepSleep();
}



//=========================== 测试函数, 快速全局刷新
// description:
//      GUI功能测试, 
//          1) 点/线/圆/字符/字符串
//          2) 保存GUI图像到文件
//
void ePaper_Test_1(void)
{
    //TODO: DispImg指向GUIImg
    DispImg = GUIImg;

    //------------ 使用GUI, 并支持局部刷新
    GUI_Init_Img(GUIImg, EPD_X_LEN, EPD_Y_LEN, Rotate0, ColorWhite); //需要交换W和H
    GUI_Clear_All(ColorWhite);
    //
    EPD_W21_Init_GUI();
    EPD_SetRAMValue_BaseMap(GUIImg); //设置背景
    // 第一个Point
    GUI_Paint_DrawPoint(720, 50, PointStyle_FillAround, ColorBlack, DotPixel_7x7);
    EPD_Dis_PartAll(DispImg);
    // vTaskDelay(800 / portTICK_PERIOD_MS);
    // // 第二个Point
    GUI_Paint_DrawPoint(200, 100, PointStyle_FillAround, ColorBlack, DotPixel_4x4);
    EPD_Dis_PartAll(DispImg);
    // vTaskDelay(800 / portTICK_PERIOD_MS);
    // 一条直线
    GUI_Paint_DrawLine(50, 80, 50, 180, LineStyle_SOLID, ColorBlack, DotPixel_2x2);
    EPD_Dis_PartAll(DispImg);
    // vTaskDelay(800 / portTICK_PERIOD_MS);
    // 一个圆
    GUI_Paint_DrawCircle(100, 200, 45, CircleStyle_Full, ColorBlack, DotPixel_3x3);
    EPD_Dis_PartAll(DispImg);
    // vTaskDelay(800 / portTICK_PERIOD_MS);
    // 一个字符
    GUI_Paint_DrawChar(450, 100, 'A', &Font16VN, ColorWhite, ColorBlack);
    GUI_Paint_DrawChar(500, 100, 'B', &Font16VN, ColorWhite, ColorBlack);
    EPD_Dis_PartAll(DispImg);
    // vTaskDelay(800 / portTICK_PERIOD_MS);
    // 一个字符串
    GUI_Paint_DrawString_EN(300, 50, "Cộng hoà xã hội chủ nghĩa Việt Nam!", &Font16VN, ColorWhite, ColorBlack);
    EPD_Dis_PartAll(DispImg);
    //
    EPD_DeepSleep();

    //------------ TestImg保存到本地
    // ePaper_SaveGUIImg();

    ESP_LOGI(TAG_EPD, ">>> GUI done");
}

//=========================== UI界面生成
// description:
//
void ePapre_Test_2_UiGenerate()
{
    //TODO: DispImg指向GUIImg
    DispImg = GUIImg;

    //------------ 使用GUI, 并支持局部刷新
    GUI_Init_Img(GUIImg, EPD_X_LEN, EPD_Y_LEN, Rotate0, ColorWhite); //需要交换W和H
    GUI_Clear_All(ColorWhite);
    EPD_W21_Init_GUI();
    EPD_SetRAMValue_BaseMap(GUIImg); //设置背景
    // 标题
    GUI_Paint_DrawString_EN(30, 20, "> ePaper Photo Frame", &Font16, ColorWhite, ColorBlack);
    GUI_Paint_DrawLine(60, 20, 60, 360, LineStyle_SOLID, ColorBlack, DotPixel_4x4);
    // 
    uint16_t x_start = 120;
    uint8_t step = 80;
    uint8_t i;
    for(i = 0; i < 9; i++)
    {
        //选择圆点
        GUI_Paint_DrawCircle(x_start + i * step, 70, 20, CircleStyel_Empty, ColorBlack, DotPixel_3x3);
        //文件名下 横线
        GUI_Paint_DrawLine(x_start + i * step + 20 + 10, 100, 
                           x_start + i * step + 20 + 10, 460, 
                           LineStyle_SOLID, ColorBlack, DotPixel_2x2);
    }
    // 
    EPD_Dis_PartAll(DispImg);
    EPD_DeepSleep();

}




////============================== EPD函数调用顺序, 代码来自GoodDispaly
////
// void __X__task_ePaper(void * arg)
// {
//     int i;

//     //--------------- Hardware init
//     EPD_HW_Init();
//     ESP_LOGI(TAG_MAIN, "init EPD IO & SPI...");


//     //--------------- EPD_W21 Show
//     //--------- Full screen refresh, fast refresh, and partial refresh demostration.

//     //------ Full clear
//     EPD_W21_Init(); //Full screen refresh initialization.
//     EPD_WhiteScreen_White(); //Clear screen function.
//     EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
//     ESP_LOGI(TAG_MAIN, ">>> full clear...");
//     vTaskDelay(2000 / portTICK_PERIOD_MS); //Delay for 2s.

//     //------ Full display(2s)
//     EPD_W21_Init(); //Full screen refresh initialization.
//     EPD_WhiteScreen_ALL(gImage_1); //To Display one image using full screen refresh.
//     EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
//     ESP_LOGI(TAG_MAIN, ">>> full display...");
//     vTaskDelay(2000 / portTICK_PERIOD_MS); //Delay for 2s.

//     //------ Fast refresh mode(1.5s)
//     EPD_W21_Init_Fast(); //Fast refresh initialization.
//     EPD_WhiteScreen_ALL_Fast(gImage_2); //To display one image using fast refresh.
//     EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
//     ESP_LOGI(TAG_MAIN, ">>> fast refresh...");
//     vTaskDelay(2000 / portTICK_PERIOD_MS); //Delay for 2s.

//     //--------- Partial refresh demostration.
// 	//Partial refresh demo support displaying a clock at 5 locations with 00:00.  If you need to perform partial refresh more than 5 locations, please use the feature of using partial refresh at the full screen demo.
// 	//After 5 partial refreshes, implement a full screen refresh to clear the ghosting caused by partial refreshes.
//     EPD_W21_Init(); //Electronic paper initialization.	
//     EPD_SetRAMValue_BaseMap(gImage_basemap); //Please do not delete the background color function, otherwise it will cause unstable display during partial refresh.
//     for(i = 0; i < 6; i++)
//         //这里48*104=4992=624*8
//         //48*104就是刷新局部图像的尺寸
//         //624*8表示用624个unint8_t填充这个局部图像
//         //Num[0]->数字0; Num[1]->数字1...
//         EPD_Dis_Part_Time(320,124+48*0, Num[1],         //x-A,y-A,DATA-A
//                         320,124+48*1, Num[0],         //x-B,y-B,DATA-B
//                         320,124+48*2, gImage_numdot, //x-C,y-C,DATA-C
//                         320,124+48*3, Num[0],        //x-D,y-D,DATA-D
//                         320,124+48*4,Num[i],48,104); //x-E,y-E,DATA-E,Resolution  32*64

//     EPD_DeepSleep();  //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
//     ESP_LOGI(TAG_MAIN, ">>> Part, display time...");
//     vTaskDelay(2000 / portTICK_PERIOD_MS); //Delay for 2s.
//     //
//     EPD_W21_Init(); //Full screen refresh initialization.
//     EPD_WhiteScreen_White(); //Clear screen function.
//     EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
//     ESP_LOGI(TAG_MAIN, ">>> full clear...");
//     vTaskDelay(2000 / portTICK_PERIOD_MS); //Delay for 2s.

//     //--------- Demo of using partial refresh to update the full screen
//     //After 5 partial refreshes, implement a full screen refresh to clear the ghosting caused by partial refreshes.
//     EPD_W21_Init(); //E-paper initialization	
//     EPD_SetRAMValue_BaseMap(gImage_p1); //Please do not delete the background color function, otherwise it will cause an unstable display during partial refresh.
//     EPD_Dis_PartAll(gImage_p1); //Image 1
//     EPD_Dis_PartAll(gImage_p2); //Image 2
//     EPD_Dis_PartAll(gImage_p3); //Image 3
//     EPD_DeepSleep();//Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
//     ESP_LOGI(TAG_MAIN, ">>> 5 partial refresh...");
//     vTaskDelay(2000 / portTICK_PERIOD_MS); //Delay for 2s.
//     //
//     EPD_W21_Init(); //Full screen refresh initialization.
//     EPD_WhiteScreen_White(); //Clear screen function.
//     EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
//     ESP_LOGI(TAG_MAIN, ">>> full clear...");
//     vTaskDelay(2000 / portTICK_PERIOD_MS); //Delay for 2s.

//     //--------- Demonstration of full screen refresh with 180-degree rotation
//     EPD_HW_Init_180(); //Full screen refresh initialization.
//     EPD_WhiteScreen_ALL(gImage_1); //To Display one image using full screen refresh.
//     EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
//     ESP_LOGI(TAG_MAIN, ">>> Demonstration refresh...");
//     vTaskDelay(2000 / portTICK_PERIOD_MS); //Delay for 2s.


//     //--------- GUI demo
//     // //GUI Demo(GUI examples can display points, lines, rectangles, circles, letters, numbers, etc).
//     // //Data initialization settings.
//     // //GUI section, height and width need to be interchanged
//     // Paint_NewImage(BlackImage, EPD_HEIGHT, EPD_WIDTH, 90, WHITE); //Set canvas parameters, GUI image rotation, please change 270 to 0/90/180/270.
//     // Paint_SelectImage(BlackImage); //Select current settings.
//     // /**************Drawing demonstration**********************/   
//     // EPD_HW_Init_GUI(); //GUI initialization.
//     // Paint_Clear(WHITE); //Clear canvas.
//     // //Point.   
//     // Paint_DrawPoint(5, 10, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT); //point 1x1.
//     // Paint_DrawPoint(5, 25, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT); //point 2x2.
//     // Paint_DrawPoint(5, 40, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT); //point 3x3.
//     // Paint_DrawPoint(5, 55, BLACK, DOT_PIXEL_4X4, DOT_STYLE_DFT); //point 4x4.
//     // //Line.
//     // Paint_DrawLine(20, 10, 70, 60, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_1X1); //1x1line 1.
//     // Paint_DrawLine(70, 10, 20, 60, BLACK, LINE_STYLE_SOLID, DOT_PIXEL_1X1); //1x1line 2.
//     // //Rectangle.
//     // Paint_DrawRectangle(20, 10, 70, 60, BLACK, DRAW_FILL_EMPTY, DOT_PIXEL_1X1); //Hollow rectangle 1.
//     // Paint_DrawRectangle(85, 10, 130, 60, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1); //Hollow rectangle 2.
//     // //Circle.
//     // Paint_DrawCircle(150, 200, 60, BLACK, DRAW_FILL_EMPTY, DOT_PIXEL_1X1); //Hollow circle.
//     // Paint_DrawCircle(250,200, 60, BLACK, DRAW_FILL_FULL, DOT_PIXEL_1X1); //solid circle.
//     // EPD_Display(BlackImage); //Display GUI image.
//     // EPD_DeepSleep();//EPD_DeepSleep,Sleep instruction is necessary, please do not delete!!!
//     // delay_s(2); //Delay for 2s.		

//     // /***********Letter demo***************************/
//     // EPD_HW_Init_GUI(); //GUI initialization.
//     // Paint_Clear(WHITE); //Clear canvas.
//     // Paint_DrawString_EN(0, 0, "Good Display", &Font8, WHITE, BLACK);  //5*8.
//     // Paint_DrawString_EN(0, 10, "Good Display", &Font12, WHITE, BLACK); //7*12.
//     // Paint_DrawString_EN(0, 25, "Good Display", &Font16, WHITE, BLACK); //11*16.
//     // Paint_DrawString_EN(0, 45, "Good Display", &Font20, WHITE, BLACK); //14*20.
//     // Paint_DrawString_EN(0, 80, "Good Display", &Font24, WHITE, BLACK); //17*24.

//     // Paint_DrawString_EN(200, 200, "Good Display", &Font8, WHITE, BLACK);  //5*8.
//     // Paint_DrawString_EN(200, 210, "Good Display", &Font12, WHITE, BLACK); //7*12.
//     // Paint_DrawString_EN(200, 225, "Good Display", &Font16, WHITE, BLACK); //11*16.
//     // Paint_DrawString_EN(200, 245, "Good Display", &Font20, WHITE, BLACK); //14*20.
//     // Paint_DrawString_EN(200, 280, "Good Display", &Font24, WHITE, BLACK); //17*24.

//     // EPD_Display(BlackImage);//Display GUI image.
//     // EPD_DeepSleep(); //EPD_DeepSleep,Sleep instruction is necessary, please do not delete!!!
//     // delay_s(2); //Delay for 2s.

//     // /*************Numbers demo************************/
//     // EPD_HW_Init_GUI(); //GUI initialization.
//     // Paint_Clear(WHITE); //Clear canvas.
//     // Paint_DrawNum(0, 0, 123456789, &Font8, WHITE, BLACK);  //5*8.
//     // Paint_DrawNum(0, 10, 123456789, &Font12, WHITE, BLACK); //7*12.
//     // Paint_DrawNum(0, 25, 123456789, &Font16, WHITE, BLACK); //11*16.
//     // Paint_DrawNum(0, 45, 123456789, &Font20, WHITE, BLACK); //14*20.
//     // Paint_DrawNum(0, 70, 123456789, &Font24, WHITE, BLACK); //17*24.

//     // Paint_DrawNum(200, 200, 123456789, &Font8, WHITE, BLACK);  //5*8.
//     // Paint_DrawNum(200, 210, 123456789, &Font12, WHITE, BLACK); //7*12.
//     // Paint_DrawNum(200, 225, 123456789, &Font16, WHITE, BLACK); //11*16.
//     // Paint_DrawNum(200, 245, 123456789, &Font20, WHITE, BLACK); //14*20.
//     // Paint_DrawNum(200, 270, 123456789, &Font24, WHITE, BLACK); //17*24.
//     // EPD_Display(BlackImage); //Display GUI image.
//     // EPD_DeepSleep();//EPD_DeepSleep,Sleep instruction is necessary, please do not delete!!!
//     // delay_s(2); //Delay for 2s.	 			

//     // //Full screen update clear the screen.
//     // EPD_HW_Init(); //Full screen refresh initialization.
//     // EPD_WhiteScreen_White(); //Clear screen function.
//     // EPD_DeepSleep(); //Enter the sleep mode and please do not delete it, otherwise it will reduce the lifespan of the screen.
//     // delay_s(2);	//Delay for 2s.	



//     //--------------- end
//     for(;;);
//     vTaskDelay(NULL);
// } 

