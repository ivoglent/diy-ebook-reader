#ifndef __GUI_PAINT_H__
#define __GUI_PAINT_H__

#include "../Fonts/fonts.h"


/************************************************************************
 * TYPES
 ************************************************************************/
typedef enum {
    MirrorNone = 0x00,
    MirrorHorizontal = 0x01,
    MirrorVertical = 0x02,
    MirrorOrigin = 0x03,
}Mirror_Img_t;

typedef enum {
    Rotate0 = 0,
    Rotate90 = 90,
    Rotate180 = 180,
    Rotate270 = 270,
}Rotate_t;

typedef enum {
    ColorWhite = 0xFF, //将设置8个pix
    ColorBlack = 0x00,
}Color_t;
#define COLOR_DEFAULT       ColorWhite

typedef enum {
    DotPixel_1x1 = 1,
    DotPixel_2x2 = 2,
    DotPixel_3x3 = 3,
    DotPixel_4x4 = 4,
    DotPixel_5x5 = 5,
    DotPixel_6x6 = 6,
    DotPixel_7x7 = 7,
    DotPixel_8x8 = 8,
    DotPixel_9x9 = 9,
    DotPixel_10x10 = 10,
    DotPixel_11x11 = 11,
    DotPixel_12x12 = 12,
}DotPixel_t;

typedef enum {
    PointStyle_FillAround = 1,
    PointStyle_FillRightup,
}PointStyle_t;
#define POINTSTYLE_DEFAULT      PointStyle_FillAround

typedef enum {
    LineStyle_SOLID = 0,
    LineStyel_Dotted,
}LineStyle_t;

typedef enum {
    CircleStyle_Full = 0,
    CircleStyel_Empty,
}CircleStyle_t;

/************************************************************************
 * DEFINES
 ************************************************************************/
#define IMG_BACKGROUND_DEFAULT      ColorWhite
#define FONT_FRONTGROUND_DEFAULT    ColorBlack
#define FONT_BACKGROUND_DEFAULT     ColorWhite


/************************************************************************
 * DECLARATION
 ************************************************************************/
void GUI_Init_Img(uint8_t *img, uint16_t width, uint16_t height, Rotate_t rotate, uint8_t color);
void GUI_Clear_All(Color_t color);
void GUI_Clear_Part(uint16_t start_x, uint16_t start_y, 
                    uint16_t end_x, uint16_t end_y, 
                    Color_t color);
int8_t GUI_Paint_DrawPoint(uint16_t x_axis, uint16_t y_axis, 
                           PointStyle_t pointStyle,
                           Color_t color, DotPixel_t pointPixel);
int8_t GUI_Paint_DrawLine(uint16_t x_start, uint16_t y_start, 
                          uint16_t x_end, uint16_t y_end,
                          LineStyle_t line_style, 
                          Color_t color, DotPixel_t dot_pixel);
int8_t GUI_Paint_DrawCircle(uint16_t x_center, uint16_t y_center, uint16_t radius,
                            CircleStyle_t circle_style, 
                            Color_t color, DotPixel_t dot_pixel);
int8_t GUI_Paint_DrawChar(uint16_t x_axis, uint16_t y_axis,
                          const uint8_t char_ascii, sFONT* font,
                          Color_t color_background, Color_t color_frontground);
int8_t GUI_Paint_DrawString_EN(uint16_t x_axis, uint16_t y_axis,
                               const char *ptr_string, sFONT* font, 
                               Color_t color_background, Color_t color_frontground);

void GUI_Paint_DrawTextUTF8(uint16_t x, uint16_t y,
                             const char *text,
                             const sFONT *font,
                             Color_t color_bg, Color_t color_fg);

int8_t GUI_Paint_DrawString_VN(uint16_t x_axis, uint16_t y_axis,
                               const char *ptr_string, sFONT* font,
                               Color_t color_background, Color_t color_frontground);


#endif
