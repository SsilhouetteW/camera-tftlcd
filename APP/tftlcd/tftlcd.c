#include "tftlcd.h"
#include "stdlib.h"
#include "font.h"
#include "usart.h"
#include "SysTick.h"

u16 FRONT_COLOR = BLACK;
u16 BACK_COLOR = WHITE;

_tftlcd_data tftlcd_data;

// LCD command write
void LCD_WriteCmd(u16 cmd)
{
#ifdef TFTLCD_HX8357DN
	TFTLCD->LCD_CMD = cmd;
#endif
}

// LCD data write
void LCD_WriteData(u16 data)
{
#ifdef TFTLCD_HX8357DN
	TFTLCD->LCD_DATA = data;
#endif
}

void LCD_WriteCmdData(u16 cmd, u16 data)
{
	LCD_WriteCmd(cmd);
	LCD_WriteData(data);
}

void LCD_WriteData_Color(u16 color)
{
#ifdef TFTLCD_HX8357DN
	TFTLCD->LCD_DATA = color >> 8;
	TFTLCD->LCD_DATA = color & 0xff;
#endif
}

// LCD data read
u16 LCD_ReadData(void)
{
#ifdef TFTLCD_HX8357DN
	return TFTLCD->LCD_DATA;
#else
	return 0;
#endif
}

// Set LCD display direction
// dir: 0=vertical, 1=horizontal
void LCD_Display_Dir(u8 dir)
{
	tftlcd_data.dir = dir;
	if (dir == 0) {
#ifdef TFTLCD_HX8357DN
		LCD_WriteCmd(0x36);
		LCD_WriteData(0x4c);
		tftlcd_data.height = 480;
		tftlcd_data.width = 320;
#endif
	} else {
#ifdef TFTLCD_HX8357DN
		LCD_WriteCmd(0x36);
		LCD_WriteData(0x2c);
		tftlcd_data.height = 320;
		tftlcd_data.width = 480;
#endif
	}
}

// Set LCD window for upcoming pixel writes
void LCD_Set_Window(u16 sx, u16 sy, u16 width, u16 height)
{
#ifdef TFTLCD_HX8357DN
	LCD_WriteCmd(0x2A);
	LCD_WriteData(sx >> 8);
	LCD_WriteData(sx & 0xFF);
	LCD_WriteData(width >> 8);
	LCD_WriteData(width & 0xFF);

	LCD_WriteCmd(0x2b);
	LCD_WriteData(sy >> 8);
	LCD_WriteData(sy & 0xFF);
	LCD_WriteData(height >> 8);
	LCD_WriteData(height & 0xFF);
	LCD_WriteCmd(0x2c);
#endif
}

// Read pixel color at point
u16 LCD_ReadPoint(u16 x, u16 y)
{
	u16 r = 0;
	if (x >= tftlcd_data.width || y >= tftlcd_data.height) return 0;
	LCD_Set_Window(x, y, x, y);

#ifdef TFTLCD_HX8357DN
	LCD_WriteCmd(0X2E);
	r = TFTLCD->LCD_DATA;
	r = TFTLCD->LCD_DATA << 8;
	r |= TFTLCD->LCD_DATA;
#endif
	return r;
}

/*******************************************************************************
* FSMC Init for TFTLCD (StdPeriph version)
*******************************************************************************/
void TFTLCD_FSMC_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	FSMC_NORSRAMTimingInitTypeDef FSMC_ReadWriteTim;
	FSMC_NORSRAMTimingInitTypeDef FSMC_WriteTim;
	FSMC_NORSRAMInitTypeDef FSMC_NORSRAMInitStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);

	// PD0,1,4,5,8,9,10,14,15 — FSMC D2,D3,NOE,NWE,D13,D14,D15,A18,LED
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_4 | GPIO_Pin_5 |
				      GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	// PE7–PE15 — FSMC D4–D12
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 |
				      GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	// PG0,12 — FSMC A10, NE4
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_12;
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	// PB0 — LCD backlight control
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// Read/Write timing
	FSMC_ReadWriteTim.FSMC_AddressSetupTime = 0x0F;
	FSMC_ReadWriteTim.FSMC_AddressHoldTime = 0x00;
	FSMC_ReadWriteTim.FSMC_DataSetupTime = 60;
	FSMC_ReadWriteTim.FSMC_BusTurnAroundDuration = 0x00;
	FSMC_ReadWriteTim.FSMC_CLKDivision = 0x00;
	FSMC_ReadWriteTim.FSMC_DataLatency = 0x00;
	FSMC_ReadWriteTim.FSMC_AccessMode = FSMC_AccessMode_A;

	// Write timing
	FSMC_WriteTim.FSMC_AddressSetupTime = 9;
	FSMC_WriteTim.FSMC_AddressHoldTime = 0x00;
	FSMC_WriteTim.FSMC_DataSetupTime = 8;
	FSMC_WriteTim.FSMC_BusTurnAroundDuration = 0x00;
	FSMC_WriteTim.FSMC_CLKDivision = 0x00;
	FSMC_WriteTim.FSMC_DataLatency = 0x00;
	FSMC_WriteTim.FSMC_AccessMode = FSMC_AccessMode_A;

	// FSMC NORSRAM init structure
	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM4;
	FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
	FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
	FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
	FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Enable;
	FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &FSMC_ReadWriteTim;
	FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &FSMC_WriteTim;

	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM4, ENABLE);
}

/*******************************************************************************
* TFTLCD Init
*******************************************************************************/
void TFTLCD_Init(void)
{
	u16 i;

	TFTLCD_FSMC_Init();
	delay_ms(50);

#ifdef TFTLCD_HX8357DN
	LCD_WriteCmd(0Xd0);
	tftlcd_data.id = LCD_ReadData();
	tftlcd_data.id = LCD_ReadData();
#endif

	printf(" LCD ID:%x\r\n", tftlcd_data.id);

#ifdef TFTLCD_HX8357DN
	LCD_WriteCmd(0x11);
	for (i = 500; i > 0; i--);

	LCD_WriteCmd(0xB9);
	LCD_WriteData(0xFF);
	LCD_WriteData(0x83);
	LCD_WriteData(0x57);
	for (i = 500; i > 0; i--);

	LCD_WriteCmd(0xB1);
	LCD_WriteData(0x00);
	LCD_WriteData(0x14);
	LCD_WriteData(0x1C);
	LCD_WriteData(0x1C);
	LCD_WriteData(0xC3);
	LCD_WriteData(0x44);
	LCD_WriteData(0x70);
	for (i = 500; i > 0; i--);

	LCD_WriteCmd(0xB4);
	LCD_WriteData(0x22);
	LCD_WriteData(0x40);
	LCD_WriteData(0x00);
	LCD_WriteData(0x2A);
	LCD_WriteData(0x2A);
	LCD_WriteData(0x20);
	LCD_WriteData(0x91);
	for (i = 500; i > 0; i--);

	LCD_WriteCmd(0x36);
	LCD_WriteData(0x4C);

	LCD_WriteCmd(0xC0);
	LCD_WriteData(0x50);
	LCD_WriteData(0x50);
	LCD_WriteData(0x01);
	LCD_WriteData(0x3C);
	LCD_WriteData(0xC8);
	LCD_WriteData(0x08);
	LCD_WriteData(0x00);
	LCD_WriteData(0x08);
	LCD_WriteData(0x04);
	for (i = 500; i > 0; i--);
	for (i = 500; i > 0; i--);

	LCD_WriteCmd(0xE0);
	LCD_WriteData(0x0B);
	LCD_WriteData(0x11);
	LCD_WriteData(0x1E);
	LCD_WriteData(0x30);
	LCD_WriteData(0x3A);
	LCD_WriteData(0x43);
	LCD_WriteData(0x4E);
	LCD_WriteData(0x56);
	LCD_WriteData(0x45);
	LCD_WriteData(0x3F);
	LCD_WriteData(0x39);
	LCD_WriteData(0x32);
	LCD_WriteData(0x2F);
	LCD_WriteData(0x2A);
	LCD_WriteData(0x29);
	LCD_WriteData(0x21);
	LCD_WriteData(0x0B);
	LCD_WriteData(0x11);
	LCD_WriteData(0x1E);
	LCD_WriteData(0x30);
	LCD_WriteData(0x3A);
	LCD_WriteData(0x43);
	LCD_WriteData(0x4E);
	LCD_WriteData(0x56);
	LCD_WriteData(0x45);
	LCD_WriteData(0x3F);
	LCD_WriteData(0x39);
	LCD_WriteData(0x32);
	LCD_WriteData(0x2F);
	LCD_WriteData(0x2A);
	LCD_WriteData(0x29);
	LCD_WriteData(0x21);
	LCD_WriteData(0x00);
	LCD_WriteData(0x01);

	LCD_WriteCmd(0x3a);
	LCD_WriteData(0x05);

	LCD_WriteCmd(0x29);
#endif

	LCD_Display_Dir(TFTLCD_DIR);
	LCD_LED = 1;
	LCD_Clear(WHITE);
}

// Clear screen
void LCD_Clear(u16 Color)
{
	u32 index = 0;
	u32 totalpoint = tftlcd_data.width;
	totalpoint *= tftlcd_data.height;

	LCD_Set_Window(0, 0, tftlcd_data.width - 1, tftlcd_data.height - 1);
	for (index = 0; index < totalpoint; index++) {
		LCD_WriteData_Color(Color);
	}
}

// Fill rectangle with single color
void LCD_Fill(u16 xState, u16 yState, u16 xEnd, u16 yEnd, u16 color)
{
	u16 i, j;
	for (i = yState; i <= yEnd; i++) {
		LCD_Set_Window(xState, i, xEnd, i);
		for (j = xState; j <= xEnd; j++) {
			LCD_WriteData_Color(color);
		}
	}
}

// Fill rectangle with color array
void LCD_Color_Fill(u16 sx, u16 sy, u16 ex, u16 ey, u16 *color)
{
	u16 height, width;
	u16 i, j;
	width = ex - sx + 1;
	height = ey - sy + 1;

	for (i = 0; i < height; i++) {
		LCD_Set_Window(sx, sy + i, ex, sy + i);
		for (j = 0; j < width; j++) {
			LCD_WriteData_Color(color[i * width + j]);
		}
	}
}

// Draw single point
void LCD_DrawPoint(u16 x, u16 y)
{
	LCD_Set_Window(x, y, x, y);
	LCD_WriteData_Color(FRONT_COLOR);
}

void LCD_DrawFRONT_COLOR(u16 x, u16 y, u16 color)
{
	LCD_Set_Window(x, y, x, y);
	LCD_WriteData_Color(color);
}

// Draw line
void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t;
	int xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int incx, incy, uRow, uCol;
	delta_x = x2 - x1;
	delta_y = y2 - y1;
	uRow = x1;
	uCol = y1;
	if (delta_x > 0) incx = 1;
	else if (delta_x == 0) incx = 0;
	else { incx = -1; delta_x = -delta_x; }
	if (delta_y > 0) incy = 1;
	else if (delta_y == 0) incy = 0;
	else { incy = -1; delta_y = -delta_y; }
	if (delta_x > delta_y) distance = delta_x;
	else distance = delta_y;
	for (t = 0; t <= distance + 1; t++) {
		LCD_DrawPoint(uRow, uCol);
		xerr += delta_x;
		yerr += delta_y;
		if (xerr > distance) {
			xerr -= distance;
			uRow += incx;
		}
		if (yerr > distance) {
			yerr -= distance;
			uCol += incy;
		}
	}
}

void LCD_DrawLine_Color(u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
	u16 t;
	int xerr = 0, yerr = 0, delta_x, delta_y, distance;
	int incx, incy, uRow, uCol;
	delta_x = x2 - x1;
	delta_y = y2 - y1;
	uRow = x1;
	uCol = y1;
	if (delta_x > 0) incx = 1;
	else if (delta_x == 0) incx = 0;
	else { incx = -1; delta_x = -delta_x; }
	if (delta_y > 0) incy = 1;
	else if (delta_y == 0) incy = 0;
	else { incy = -1; delta_y = -delta_y; }
	if (delta_x > delta_y) distance = delta_x;
	else distance = delta_y;
	for (t = 0; t <= distance + 1; t++) {
		LCD_DrawFRONT_COLOR(uRow, uCol, color);
		xerr += delta_x;
		yerr += delta_y;
		if (xerr > distance) {
			xerr -= distance;
			uRow += incx;
		}
		if (yerr > distance) {
			yerr -= distance;
			uCol += incy;
		}
	}
}

// Draw cross sign
void LCD_DrowSign(uint16_t x, uint16_t y, uint16_t color)
{
	LCD_DrawLine_Color(x - 5, y, x + 5, y, color);
	LCD_DrawLine_Color(x, y - 5, x, y + 5, color);
}

// Draw rectangle
void LCD_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	LCD_DrawLine(x1, y1, x2, y1);
	LCD_DrawLine(x1, y1, x1, y2);
	LCD_DrawLine(x1, y2, x2, y2);
	LCD_DrawLine(x2, y1, x2, y2);
}

// Draw circle
void LCD_Draw_Circle(u16 x0, u16 y0, u8 r)
{
	int a, b;
	int di;
	a = 0;
	b = r;
	di = 3 - (r << 1);
	while (a <= b) {
		LCD_DrawPoint(x0 + a, y0 - b);
		LCD_DrawPoint(x0 + b, y0 - a);
		LCD_DrawPoint(x0 + b, y0 + a);
		LCD_DrawPoint(x0 + a, y0 + b);
		LCD_DrawPoint(x0 - a, y0 + b);
		LCD_DrawPoint(x0 - b, y0 + a);
		LCD_DrawPoint(x0 - a, y0 - b);
		LCD_DrawPoint(x0 - b, y0 - a);
		a++;
		if (di < 0) di += 4 * a + 6;
		else {
			di += 10 + 4 * (a - b);
			b--;
		}
	}
}

// Show single character (column-major rendering matches PC2LCD2002 font data)
void LCD_ShowChar(u16 x, u16 y, u8 num, u8 size, u8 mode)
{
	u8 temp, t, t1;
	u16 y0 = y;
	u8 csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2);

	num = num - ' ';  // offset into font array
	for (t = 0; t < csize; t++) {
		if (size == 12)      temp = asc2_1206[num][t];
		else if (size == 16) temp = asc2_1608[num][t];
		else if (size == 24) temp = asc2_2412[num][t];
		else if (size == 32) temp = asc2_3216[num][t];
		else return;

		for (t1 = 0; t1 < 8; t1++) {
			if (temp & 0x80)
				LCD_DrawFRONT_COLOR(x, y, FRONT_COLOR);
			else if (mode == 0)
				LCD_DrawFRONT_COLOR(x, y, BACK_COLOR);
			temp <<= 1;
			y++;
			if (y >= tftlcd_data.height) return;
			if ((y - y0) == size) {
				y = y0;
				x++;
				if (x >= tftlcd_data.width) return;
				break;
			}
		}
	}
}
// Show number
void LCD_ShowNum(u16 x, u16 y, u32 num, u8 len, u8 size)
{
	LCD_ShowxNum(x, y, num, len, size, 0);
}

// Show number with options
static u32 lcd_pow(u8 m, u8 n)
{
	u32 result = 1;
	while (n--) result *= m;
	return result;
}

void LCD_ShowxNum(u16 x, u16 y, u32 num, u8 len, u8 size, u8 mode)
{
	u8 t, temp;
	for (t = 0; t < len; t++) {
		temp = (num / lcd_pow(10, len - t - 1)) % 10;
		if (mode == 0 && t < len - 1 && temp == 0) {
			LCD_ShowChar(x + (size / 2) * t, y, ' ', size, 0);
		} else {
			LCD_ShowChar(x + (size / 2) * t, y, temp + '0', size, mode);
		}
	}
}

// Display picture from RGB565 byte array
void LCD_ShowPicture(u16 x, u16 y, u16 wide, u16 high, u8 *pic)
{
	u16 i, j;
	u16 temp = 0;
	long tmp = 0;

	for (i = 0; i < high; i++) {
		for (j = 0; j < wide; j++) {
			temp = pic[tmp + 1];
			temp = temp << 8;
			temp = temp | pic[tmp];
			LCD_DrawFRONT_COLOR(x + j, y + i, temp);
			tmp += 2;
		}
	}
}

// Show string
void LCD_ShowString(u16 x, u16 y, u16 width, u16 height, u8 size, u8 *p)
{
	u8 x0 = x;
	width += x;
	height += y;
	while ((*p <= '~') && (*p >= ' ')) {
		if (x >= width) { x = x0; y += size; }
		if (y >= height) break;
		LCD_ShowChar(x, y, *p, size, 0);
		x += size / 2;
		p++;
	}
}

// Show Chinese font (16x16)
void LCD_ShowFontHZ(u16 x, u16 y, u8 *cn)
{
	// Placeholder for Chinese font support
	// Requires Chinese font library
}
