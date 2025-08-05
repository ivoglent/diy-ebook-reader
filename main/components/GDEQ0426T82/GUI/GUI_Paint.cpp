/************************************************
 * ** 维护了一个.c全局变量Paint
 * ** 提供GUI绘制功能
 * ** 
 * ** 提供菜单显示功能
 * 
 ************************************************/

#include <string.h>
#include <inttypes.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_system.h"
// #include "driver/spi_master.h"
// #include "driver/gpio.h"
#include "esp_log.h"
//
#include "GUI_Paint.h"

#include "../Fonts/font_viet_lookup.h"

/************************************************************************
 * DEFINES
 ************************************************************************/
#define TAG_GUI     "EPD GUI"

#define CHECK_X_Y           if(x > Paint.Width || y > Paint.Height) \
                            { \
                                ESP_LOGI(TAG_GUI, "!!! error x or y !!!"); \
                                return -1; \
                            } \

//=========================== GUI缓存Paint
// .Image, 图像数组的指针
//      指向一个480*800/8的数组,
//      X方向长度800 pixel, 将在该方向上用byte压缩, 因此有800/8
//      Y方向上长度480 pixel
//
typedef struct {
    uint8_t *Image; //图像数组的指针
    
    uint16_t _Y; //Width; //显示的宽=480, 用来标记坐标位置(x,y)
    uint16_t _X; //Height; //显示的高=800
    //在数组中
    uint16_t Byte_Y; //WidthByte; //图像在数组中          列向数据个数(1byte对应8pix -> Width/8, 也就是一行)
    uint16_t Byte_X; //HeightByte; //图像在数组中         行数
    //在EPD中
    uint16_t RAM_Y; //WidthMemory; //图像在EPD缓存中
    uint16_t RAM_X; //HeightMemory; //图像在EPD缓存中
    
    uint16_t Color; 
    
    Rotate_t Rotate;
    uint16_t Mirror;
} Paint_t;


/************************************************************************
 * VARIABLES
 ************************************************************************/
Paint_t Paint;



/************************************************************************
 * DECLARATION
 ************************************************************************/
int8_t __Paint_SetPixel(uint16_t x_axis, uint16_t y_axis, Color_t color);



/************************************************************************
 * FUNCTIONS
 ************************************************************************/

int get_glyph_index(uint16_t unicode_char) {
    for (int i = 0; i < FONT_VIET_16X16_NUM_CHARS; i++) {
        if (unicode_lookup_table[i] == unicode_char) return i;
    }
    return -1;  // not found
}

uint16_t utf8_decode(const char **ptr) {
    const uint8_t *s = (const uint8_t *)(*ptr);
    uint16_t cp;

    if (s[0] < 0x80) {
        cp = s[0];
        (*ptr) += 1;
    } else if ((s[0] & 0xE0) == 0xC0) {
        cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        (*ptr) += 2;
    } else if ((s[0] & 0xF0) == 0xE0) {
        cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        (*ptr) += 3;
    } else {
        cp = '?';
        (*ptr) += 1;
    }

    return cp;
}

//=========================== 初始化.c全局变量Paint
// description:
//      
//
// void GUI_Init_Img(uint8_t *img, uint16_t width, uint16_t height, Rotate_t rotate, uint8_t color)
void GUI_Init_Img(uint8_t *img, uint16_t xAaxis, uint16_t yAaxis, Rotate_t rotate, uint8_t color)
// void GUI_Init_Img(uint8_t *img, uint16_t xAaxis, uint16_t yAaxis, uint8_t color)
{
    //------------ 更新.Image
    Paint.Image = NULL;
    Paint.Image = img;

    Paint.RAM_X = xAaxis;
    Paint.RAM_Y = yAaxis;

    //X-Wdith, Y-Height
    // X方向上, 在数组中按8pix构成1byte进行压缩
    // Y方向上, 一行仍保持一行
    Paint.Byte_X = (xAaxis % 8 == 0) ? (xAaxis / 8) : (xAaxis / 8 + 1); //TODO: 为什么要 width%8 
    Paint.Byte_Y = yAaxis;

    Paint.Color = color;

    Paint.Rotate = rotate;
    Paint.Mirror = MirrorNone;

    if((rotate == Rotate0) || (rotate == Rotate180))
    {
        Paint._X = xAaxis;
        Paint._Y = yAaxis;
    }
    else
    {
        Paint._X = yAaxis;
        Paint._Y = xAaxis;
    }

}


//=========================== 清除全部.Image
// description:
//      
//
void GUI_Clear_All(Color_t color)
{
    uint16_t x, y;
    uint32_t idx;

    for(y = 0; y < Paint.Byte_Y; y++)
    {
        for(x = 0; x < Paint.Byte_X; x++)
        {
            idx = x + y * Paint.Byte_X;
            Paint.Image[idx] = color;
        }
    }

}

//=========================== 清除部分.Image
// description:
//      
//
void GUI_Clear_Part(uint16_t start_x, uint16_t start_y, 
                    uint16_t end_x, uint16_t end_y, 
                    Color_t color)
{
    uint16_t x, y;

    if((start_x > end_x) || (start_y > end_y))
    {
        ESP_LOGI(TAG_GUI, "!!! error x or y !!!");
        return;
    }

    for(y = start_y; y < end_y; y++)
    {
        for(x = start_x; x < end_x; x++)
        {
            __Paint_SetPixel(x, y, color);
        }
    }
}


//=========================== 在Paint.Image上, 绘制pixels
// description:
//      
//
int8_t __Paint_SetPixel(uint16_t x_axis, uint16_t y_axis, Color_t color)
{
    uint16_t x_now, y_now;
    uint32_t idx;
    uint8_t img_byte_data;

    if(x_axis > Paint._X || y_axis > Paint._Y)
    {
        ESP_LOGI(TAG_GUI, "!!! SetPixel - error x or y !!!");
        return -1;
    }

    switch (Paint.Rotate)
    {
        case Rotate0:
            x_now = x_axis;
            y_now = y_axis;
            break;
        case Rotate90:
            x_now = Paint.RAM_X - 1 - y_axis; 
            y_now = x_axis;
            break;
        case Rotate180:
            x_now = Paint.RAM_X - 1 - x_axis; 
            y_now = Paint.RAM_Y - 1 - y_axis;
            break;
        case Rotate270:
            x_now = y_axis; 
            y_now = Paint.RAM_Y - 1 - x_axis;
            break;

        default:
            ESP_LOGI(TAG_GUI, "!!! error .Rotate !!!");
            return -1;
    }
    //
    switch (Paint.Mirror)
    {
        case MirrorNone:
            break;
        case MirrorHorizontal:
            x_now = Paint.RAM_X - 1 - x_now;
            break;
        case MirrorVertical:
            y_now = Paint.RAM_Y - 1 - y_now;
            break;
        case MirrorOrigin:
            x_now = Paint.RAM_X - 1 - x_now;
            y_now = Paint.RAM_Y - 1 - y_now;
            break;

        default:
            ESP_LOGI(TAG_GUI, "!!! error .Mirror !!!");
            return -1;
    }
    //
    if(x_now > Paint.RAM_X || y_now > Paint.RAM_Y)
    {
        ESP_LOGI(TAG_GUI, "!!! get error x_now or y_now !!!");
        return -1;
    }

    idx = x_now / 8 + y_now * Paint.Byte_X;
    img_byte_data = Paint.Image[idx];
    //
    if(color == ColorBlack)
        Paint.Image[idx] = img_byte_data & ~(0x80 >> (x_now % 8)); //置0是black
    else
        Paint.Image[idx] = img_byte_data | (0x80 >> (x_now % 8)); //置1是white


    return 0;
}

//=========================== 在Paint.Image上, 绘制 点
// description:
//      
//
int8_t GUI_Paint_DrawPoint(uint16_t x_axis, uint16_t y_axis, 
                           PointStyle_t pointStyle, 
                           Color_t color, DotPixel_t pointPixel)
{
    int16_t x_dir, y_dir;

    if(x_axis > Paint._X || y_axis > Paint._Y)
    {
        ESP_LOGI(TAG_GUI, "!!! DrawPoint: error x or y !!!");
        return -1;
    }

    if(pointStyle == PointStyle_FillAround)
    {
        for(x_dir = 0; x_dir < 2 * pointPixel - 1; x_dir++)
        {
            for(y_dir = 0; y_dir < 2 * pointPixel - 1; y_dir++)
            {
                if((int16_t)x_axis + x_dir - (int16_t)pointPixel < 0 || (int16_t)y_axis + y_dir - (int16_t)pointPixel < 0) //TODO: 这里涉及 无符号数 和 有符号数 相加问题
                    break;
                //
                __Paint_SetPixel(x_axis - pointPixel + x_dir, 
                                   y_axis - pointPixel + y_dir,
                                   color);
            }
        }
    }
    else
    {
        for(x_dir = 0; x_dir < pointPixel; x_dir++)
        {
            for(y_dir = 0; y_dir < pointPixel; y_dir++)
            {
                __Paint_SetPixel(x_axis - 1 + x_dir, 
                                   y_axis - 1 + y_dir,
                                   color);
            }
        }
    }

    return 0;
}

//=========================== 在Paint.Image上, 绘制 线
// description: 基于Bresenham直线算法
//      
//
int8_t GUI_Paint_DrawLine(uint16_t x_start, uint16_t y_start, 
                          uint16_t x_end, uint16_t y_end,
                          LineStyle_t line_style, 
                          Color_t color, DotPixel_t dot_pixel)
{
    uint16_t x_now, y_now; //当前(x, y)
    int16_t dx, dy; //起止点位移
    int8_t sx, sy; //x/y方向上步进
    int16_t err, err2;
    uint16_t line_len;

    x_now = x_start; y_now = y_start;

    //dx=abs(x_start - x_end)   //先取绝对值, 再决定是否取负
    if(x_start < x_end)
    { dx = x_end - x_start; sx = 1; }
    else
    { dx = x_start - x_end; sx = -1; }
    //dy=-abs(y_start - y_end)
    if(y_start < y_end)
    { dy = y_start - y_end; sy = 1; }
    else
    { dy =  y_end - y_start; sy = -1; }

    err = dx + dy; //对应: e_xy

    line_len = 0;

    //绘制直线
    for(;;)
    {
        //画当前坐标点
        line_len++;
        if(line_style == LineStyle_SOLID)
            GUI_Paint_DrawPoint(x_now, y_now, POINTSTYLE_DEFAULT, color, dot_pixel);
        else if(line_len %3 == 0)
        {
            GUI_Paint_DrawPoint(x_now, y_now, POINTSTYLE_DEFAULT, COLOR_DEFAULT, dot_pixel);
            line_len = 0;
        }
        //迭代dx/dy以及x_now/y_now
        err2 = 2 * err;
        if(err2 >= dy) //对应: 判断e_xy + e_x > 0
        {
            if(x_now == x_end)
                break;
            //
            err += dy;
            x_now += sx;
        }
        if(err2 <= dx) //对应: 判断e_xy + e_y < 0
        {
            if(y_now == y_end)
                break;
            //
            err += dx;
            y_now += sy;
        }
    }

    return 0;
}


//=========================== 在Paint.Image上, 绘制 正圆
// description: 基于Bresenham四分圆算法
//      
//
int8_t GUI_Paint_DrawCircle(uint16_t x_center, uint16_t y_center, uint16_t radius,
                            CircleStyle_t circle_style, 
                            Color_t color, DotPixel_t dot_pixel)
{
    int16_t r = radius;
    int16_t x = -r, y = 0, err = 2 - 2 * r; //bottom left to top right
    int16_t i;

    if(circle_style == CircleStyel_Empty)
    {
        do{
            //绘制四分点
            __Paint_SetPixel(x_center - x, y_center + y, color); // I.      Quadrant +x +y
            __Paint_SetPixel(x_center - y, y_center - x, color); // II.     Quadrant -x +y
            __Paint_SetPixel(x_center + x, y_center - y, color); // III.    Quadrant -x -y
            __Paint_SetPixel(x_center + y, y_center + x, color); // IV.     Quadrant +x -y
            // ESP_LOGI(TAG_GUI, "(%d,%d),(%d,%d),(%d,%d),(%d,%d)", 
            //                     x_center - x, y_center + y,
            //                     x_center - y, y_center - x,
            //                     x_center + x, y_center - y,
            //                     x_center + y, y_center + x);
            //迭代
            r = err;
            if(r <= y) //e_xy + e_y < 0
                err += ++y*2 + 1;
            if(r > x || err >y) //e_xy + e_x > 0 OR no 2nd y-step
                err += ++x*2 + 1; //-> x-step now 
        }while(x < 0);
    }
    else if(circle_style == CircleStyle_Full)
    {
        do{
            //绘制四分点
            for(i = 0; i < -x; i++) //按四个象限填充
            {
                __Paint_SetPixel(x_center - x - i, y_center + y, color); // I.      Quadrant +x +y
                __Paint_SetPixel(x_center - y, y_center - x -i, color); // II.     Quadrant -x +y
                __Paint_SetPixel(x_center + x + i, y_center - y, color); // III.    Quadrant -x -y
                __Paint_SetPixel(x_center + y, y_center + x + i, color); // IV.     Quadrant +x -y 
            }
            //迭代
            r = err;
            if(r <= y) //e_xy + e_y < 0
                err += ++y*2 + 1;
            if(r > x || err >y) //e_xy + e_x > 0 OR no 2nd y-step
                err += ++x*2 + 1; //-> x-step now 
        }while(x < 0);
    }
    else
        return -1;

    return 0;
}


int GUI_Paint_DrawCharUTF8(uint16_t x, uint16_t y, uint16_t unicode_char,
                            const sFONT *font,
                            Color_t color_background, Color_t color_frontground)
{
    int glyph_index = get_glyph_index(unicode_char);
    if (glyph_index < 0) return -1;

    uint16_t glyph_width = font->Width;
    uint16_t glyph_height = font->Height;
    uint16_t bytes_per_row = (glyph_width + 7) / 8;
    uint16_t glyph_size = bytes_per_row * glyph_height;

    const uint8_t *glyph = &font->table[glyph_index * glyph_size];

    for (int row = 0; row < glyph_height; row++) {
        for (int col = 0; col < glyph_width; col++) {
            uint8_t byte = glyph[row * bytes_per_row + col / 8];
            uint8_t bit = 0x80 >> (col % 8);
            if (byte & bit)
                __Paint_SetPixel(x + col, y + row, color_frontground);
            else if (color_background != 0xFF)
                __Paint_SetPixel(x + col, y + row, color_background);
        }
    }

    return 0;
}



void GUI_Paint_DrawTextUTF8(uint16_t x, uint16_t y,
                             const char *text,
                             const sFONT *font,
                             Color_t color_bg, Color_t color_fg)
{
    while (*text) {
        const char *p = text;
        uint16_t unicode = utf8_decode(&text);
        GUI_Paint_DrawCharUTF8(x, y, unicode, font, color_bg, color_fg);
        x += font->Width;  // move to next char
    }
}
//=========================== 在Paint.Image上, 绘制 字符
// description: 
//      存在字符方向问题已明确, 需要用判断区分像素更新顺序;
//      当前固定像素更新方向 - 沿着Y轴, 从左到右
//
// input:
//      font, 字体数据, 存放在一维数组x_Table[]中
//            x_Table[]每个元素对应8 pixel
//            字体号数 = 行数 = Height
//            一行像素 = Width <= n个char
//
int8_t GUI_Paint_DrawChar(uint16_t x_axis, uint16_t y_axis,
                          const uint8_t char_ascii, sFONT* font,
                          Color_t color_background, Color_t color_frontground)
{

    uint32_t char_offset;
    const uint8_t * char_data;
    uint16_t x_font, y_font; //字体像素迭代的中间变量 x_font对应Width, y_font对应Height
    uint16_t *x_add, *y_add; //X/Y轴方向上增量
    int8_t x_dir, y_dir; //字体方向
    uint16_t x_img = x_axis, y_img = y_axis;

    // //需要指定屏幕的坐标
    // uint16_t x_img, y_img;
    // //沿X方向
    // x_img = x_axis + x_font; y_img = y_axis + y_font; //一, <- v
    // x_img = x_axis - x_font; y_img = y_axis + y_font; //二, -> v (正)
    // x_img = x_axis + x_font; y_img = y_axis - y_font; //四, <- ^
    // x_img = x_axis - x_font; y_img = y_axis - y_font; //三, -> ^
    // //沿Y方向
    // x_img = x_axis + y_font; y_img = y_axis + x_font; //一, -> v (正)
    // x_img = x_axis - y_font; y_img = y_axis + x_font; //二, -> ^
    // x_img = x_axis + y_font; y_img = y_axis - x_font; //四, <- v
    // x_img = x_axis - y_font; y_img = y_axis - x_font; //三, <- ^

    // dir
    if(0) //沿X方向
    {
        x_add = &x_font; //x方向将被施加Width
        y_add = &y_font; //y方向将被施加Height

        x_dir = -1; y_dir = 1;
    }
    else if(1) //沿Y方向
    {
        x_add = &y_font;
        y_add = &x_font;

        x_dir = 1; y_dir = 1;
    }

    if(x_axis > Paint._X || y_axis > Paint._Y)
    {
        return -1;
    }

    char_offset = (char_ascii - ' ') * font->Height * (font->Width / 8 + (font->Width % 8 ? 1 : 0)); //(char_ascii - ' ')拿到字符的序号
    char_data = &font->table[char_offset]; //table是个一维数组, 包含ASCII字符

    //绘制char
    if(color_background == FONT_BACKGROUND_DEFAULT) //背景色为 白
    {
        for(y_font = 0; y_font < font->Height; y_font++) //绘制height行
        {
            for(x_font = 0; x_font < font->Width; x_font++) //绘制一行中的width列
            {
                if(*char_data & (0x80 >> (x_font % 8))) //展开成8bit取其中1bit, 设置对应颜色
                {
                    __Paint_SetPixel(x_axis + (*x_add) * x_dir, y_axis + (*y_add) * y_dir, color_frontground); //(x_axis, y_axis)是起始点
                }
                //
                if(x_font % 8 == 7) //处理完8bits后, 移动到下一个table数据
                    char_data++;
            } //绘制完一行
            if(font->Width % 8 != 0) //如果一行数据量font->Width不够n个char, 就跳过当前table数据
                char_data++;
        }
    }
    else if(color_background == ColorBlack) //背景色为 黑
    {
        for(y_font = 0; y_font < font->Height; y_font++)
        {
            for(x_font = 0; x_font < font->Width; x_font++) //绘制一行
            {
                if(*char_data & (0x80 >> (x_font % 8)))
                    __Paint_SetPixel(x_axis + (*x_add) * x_dir, y_axis + (*y_add) * y_dir, color_frontground);
                else
                    __Paint_SetPixel(x_axis + (*x_add) * x_dir, y_axis + (*y_add) * y_dir, color_background);
                //
                if(x_font % 8 == 7)
                    char_data++;
            }
            if(font->Width % 8 != 0)
                char_data++;
        }
    }
    else
        return -1;


    return 0;
}

//=========================== 在Paint.Image上, 绘制 字符串
// description: 
//      
//
int8_t GUI_Paint_DrawString_EN(uint16_t x_axis, uint16_t y_axis,
                               const char *ptr_string, sFONT* font, 
                               Color_t color_background, Color_t color_frontground)
{
    uint16_t x_now = x_axis;
    uint16_t y_now = y_axis;

    if(x_axis > Paint._X || y_axis > Paint._Y)
    {
        ESP_LOGI(TAG_GUI, "!!! error x_axis or y_axis !!!");
        return -1;
    }

    while (*ptr_string != '\0')
    {
        if(0) //沿X方向
        {

            //X方向坐标越界处理
            if((x_now + font->Width) > Paint._X)
            {
                //X从头开始, Y从下一行开始
                x_now = x_axis;
                y_now += font->Width;
            }
            //Y方向坐标越界处理
            if((y_now + font->Height) > Paint._Y)
            {
                //从第一个字符开始
                x_now = x_axis;
                y_now = y_axis;
            }

            GUI_Paint_DrawChar(x_now, y_now, 
                               *ptr_string,
                               font, color_background, color_frontground);

            //取下一个字符
            ptr_string++;
            //更新坐标
            x_now += font->Width; //更新X坐标
            // y_now = y_now; //更新Y坐标
        }
        else if(1) //沿Y方向
        {
           //X方向坐标越界处理
            if((x_now + font->Height) > Paint._X)
            {
                //X从下一行开始, Y从头开始
                x_now += font->Height;
                y_now = y_axis;
            }
            //Y方向坐标越界处理
            if((y_now + font->Width) > Paint._Y)
            {
                //从第一个字符开始
                x_now = x_axis;
                y_now = y_axis;
            }
            // ESP_LOGI(TAG_GUI, "set str %c, at (%d, %d)", *ptr_string, x_now, y_now);

            GUI_Paint_DrawChar(x_now, y_now, *ptr_string, 
                              font, color_background, color_frontground);

            //取下一个字符
            ptr_string++;
            //更新坐标
            //x_now += font->Width; //更新X坐标
            y_now += font->Width; //更新Y坐标
        }
        else
            return -1;
    }
    

    return 0;
}



int8_t GUI_Paint_DrawString_VN(uint16_t x_axis, uint16_t y_axis,
                               const char *ptr_string, sFONT* font,
                               Color_t color_background, Color_t color_frontground)
{
    uint16_t x_now = x_axis;
    uint16_t y_now = y_axis;

    if(x_axis > Paint._X || y_axis > Paint._Y)
    {
        ESP_LOGI(TAG_GUI, "!!! error x_axis or y_axis !!!");
        return -1;
    }

    while (*ptr_string != '\0')
    {
        if(0) //沿X方向
        {

            //X方向坐标越界处理
            if((x_now + font->Width) > Paint._X)
            {
                //X从头开始, Y从下一行开始
                x_now = x_axis;
                y_now += font->Width;
            }
            //Y方向坐标越界处理
            if((y_now + font->Height) > Paint._Y)
            {
                //从第一个字符开始
                x_now = x_axis;
                y_now = y_axis;
            }

            GUI_Paint_DrawCharUTF8(x_now, y_now,
                               *ptr_string,
                               font, color_background, color_frontground);

            //取下一个字符
            ptr_string++;
            //更新坐标
            x_now += font->Width; //更新X坐标
            // y_now = y_now; //更新Y坐标
        }
        else if(1) //沿Y方向
        {
           //X方向坐标越界处理
            if((x_now + font->Height) > Paint._X)
            {
                //X从下一行开始, Y从头开始
                x_now += font->Height;
                y_now = y_axis;
            }
            //Y方向坐标越界处理
            if((y_now + font->Width) > Paint._Y)
            {
                //从第一个字符开始
                x_now = x_axis;
                y_now = y_axis;
            }
            // ESP_LOGI(TAG_GUI, "set str %c, at (%d, %d)", *ptr_string, x_now, y_now);

            GUI_Paint_DrawCharUTF8(x_now, y_now, *ptr_string,
                              font, color_background, color_frontground);

            //取下一个字符
            ptr_string++;
            //更新坐标
            //x_now += font->Width; //更新X坐标
            y_now += font->Width; //更新Y坐标
        }
        else
            return -1;
    }


    return 0;
}













