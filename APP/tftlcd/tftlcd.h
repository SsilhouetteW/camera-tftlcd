#ifndef _tftlcd_H
#define _tftlcd_H
#include "system.h"

//LCD controller selection
//#define TFTLCD_HX8357D
//#define TFTLCD_HX8352C
//#define TFTLCD_ILI9341
//#define TFTLCD_ILI9327
//#define TFTLCD_ILI9486
//#define TFTLCD_R61509V
//#define TFTLCD_R61509VN
//#define TFTLCD_R61509V3
//#define TFTLCD_ST7793
//#define TFTLCD_NT35510
#define TFTLCD_HX8357DN
//#define TFTLCD_ILI9325
//#define TFTLCD_SSD1963
//#define TFTLCD_ILI9481
//#define TFTLCD_R61509VE
//#define TFTLCD_SSD1963N
//#define TFTLCD_ILI9488
//#define TFTLCD_ILI9806

#define TFTLCD_DIR	0	//0=vertical 1=horizontal default vertical

#define	LCD_LED PBout(0) //LCD backlight PB0

//TFTLCD address structure
typedef struct
{
	u16 LCD_CMD;
	u16 LCD_DATA;
}TFTLCD_TypeDef;

//NOR/SRAM Bank1.sector4, A10 as data/command select
//16-bit mode: STM32 internal address shifts right by 1
#define TFTLCD_BASE        ((u32)(0x6C000000 | 0x000007FE))
#define TFTLCD             ((TFTLCD_TypeDef *) TFTLCD_BASE)

//LCD key parameters
typedef struct
{
	u16 width;			//LCD width
	u16 height;			//LCD height
	u16 id;				//LCD ID
	u8  dir;            //LCD direction
}_tftlcd_data;

extern _tftlcd_data tftlcd_data;
extern u16 FRONT_COLOR;
extern u16 BACK_COLOR;

//Color definitions RGB565
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000
#define BLUE         	 0x001F
#define BRED             0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40
#define BRRED 			 0XFC07
#define GRAY  			 0X8430
#define DARKBLUE      	 0X01CF
#define LIGHTBLUE      	 0X7D7C
#define GRAYBLUE       	 0X5458
#define LIGHTGREEN     	 0X841F
#define LIGHTGRAY        0XEF5B
#define LGRAY 			 0XC618
#define LGRAYBLUE        0XA651
#define LBBLUE           0X2B12

void LCD_WriteCmd(u16 cmd);
void LCD_WriteData(u16 data);
void LCD_WriteCmdData(u16 cmd,u16 data);
void LCD_WriteData_Color(u16 color);

void TFTLCD_Init(void);
void LCD_Set_Window(u16 sx,u16 sy,u16 width,u16 height);
void LCD_Display_Dir(u8 dir);
void LCD_Clear(u16 Color);
void LCD_Fill(u16 xState,u16 yState,u16 xEnd,u16 yEnd,u16 color);
void LCD_Color_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 *color);
void LCD_DrawPoint(u16 x,u16 y);
void LCD_DrawFRONT_COLOR(u16 x,u16 y,u16 color);
u16 LCD_ReadPoint(u16 x,u16 y);
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_DrawLine_Color(u16 x1, u16 y1, u16 x2, u16 y2,u16 color);
void LCD_DrowSign(uint16_t x, uint16_t y, uint16_t color);
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2);
void LCD_Draw_Circle(u16 x0,u16 y0,u8 r);
void LCD_ShowChar(u16 x,u16 y,u8 num,u8 size,u8 mode);
void LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size);
void LCD_ShowxNum(u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode);
void LCD_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p);
void LCD_ShowFontHZ(u16 x, u16 y, u8 *cn);
void LCD_ShowPicture(u16 x, u16 y, u16 wide, u16 high,u8 *pic);

#endif
