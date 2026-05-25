#include "system.h"
#include "SysTick.h"
#include "usart.h"
#include "led.h"
#include "tftlcd.h"
#include "picture.h"
#include "wifi_config.h"
#include "wifi_function.h"
#include <string.h>
#include <stdio.h>

/* ============================================================
 * USER CONFIGURATION — fill in your WiFi credentials here
 * ============================================================ */
#define WIFI_SSID       "iPhone (8)"
#define WIFI_PASSWORD   "wxjywmmszg"
/* ============================================================ */

#define IMG_WIDTH   160
#define IMG_HEIGHT  120
#define IMG_SIZE    (IMG_WIDTH * IMG_HEIGHT * 2)

static u8  image_buf[IMG_SIZE];
static u32 image_index = 0;
static u8  image_complete = 0;
static u32 frame_count = 0;

static char g_ip_str[16];

int main()
{
	char *ip_start;
	char *ip_end;
	char status_line1[32];
	char status_line2[32];
	char status_line3[32];

	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	USART1_Init(115200);
	LED_Init();
	TFTLCD_Init();

	FRONT_COLOR = BLACK;
	BACK_COLOR = WHITE;

	printf("\r\n========================================\r\n");
	printf("   WiFi Camera TFTLCD Project\r\n");
	printf("========================================\r\n");

	/* 1. Init ESP8266 in STA mode */
	LCD_Clear(WHITE);
	LCD_ShowString(10, 10,  tftlcd_data.width, tftlcd_data.height, 16,
		       (u8 *)"WiFi Camera TFTLCD");
	LCD_ShowString(10, 40,  tftlcd_data.width, tftlcd_data.height, 16,
		       (u8 *)"Init ESP8266...");
	LCD_ShowString(10, 120, tftlcd_data.width, tftlcd_data.height, 12,
		       (u8 *)"www.prechin.cn");

	printf("[1/3] Enabling ESP8266...\r\n");
	ESP8266_Choose(ENABLE);
	ESP8266_Rst();
	delay_ms(2000);

	printf("[2/3] AT Test...\r\n");
	ESP8266_AT_Test();

	printf("[3/3] STA Mode + TCP Server...\r\n");
	ESP8266_Init_STA_TCP_Server(WIFI_SSID, WIFI_PASSWORD);

	/* 2. Check WiFi connection and get IP */
	strcpy(g_ip_str, "?.?.?.?");

	printf("[WiFi] Checking connection...\r\n");
	ESP8266_Cmd("AT+CWJAP?", "OK", NULL, 2000);
	printf("[WiFi] CWJAP: %s\r\n",
	       strEsp8266_Fram_Record.Data_RX_BUF);

	ESP8266_Cmd("AT+CIFSR", "OK", NULL, 3000);
	printf("[IP] CIFSR: %s\r\n",
	       strEsp8266_Fram_Record.Data_RX_BUF);

	/* Try +CIFSR:STAIP,"x.x.x.x" format */
	ip_start = strstr(strEsp8266_Fram_Record.Data_RX_BUF, "+CIFSR:STAIP,\"");
	if (ip_start == NULL)
		ip_start = strstr(strEsp8266_Fram_Record.Data_RX_BUF, "STAIP,\"");
	if (ip_start != NULL) {
		while (*ip_start != '"' && *ip_start != '\0') ip_start++;
		if (*ip_start == '"') {
			ip_start++;
			ip_end = strchr(ip_start, '"');
			if (ip_end != NULL) {
				*ip_end = '\0';
				strcpy(g_ip_str, ip_start);
			}
		}
	}
	/* Fallback: bare IP in quotes */
	if (g_ip_str[0] == '?') {
		ip_start = strstr(strEsp8266_Fram_Record.Data_RX_BUF, "\"192.");
		if (ip_start == NULL)
			ip_start = strstr(strEsp8266_Fram_Record.Data_RX_BUF, "\"10.");
		if (ip_start == NULL)
			ip_start = strstr(strEsp8266_Fram_Record.Data_RX_BUF, "\"172.");
		if (ip_start != NULL) {
			ip_start++;
			ip_end = strchr(ip_start, '"');
			if (ip_end != NULL) {
				*ip_end = '\0';
				strcpy(g_ip_str, ip_start);
			}
		}
	}

	printf("\r\n========================================\r\n");
	printf("  STA Mode Ready!\r\n");
	printf("  WiFi:  %s\r\n", WIFI_SSID);
	printf("  IP:    %s\r\n", g_ip_str);
	printf("  Port:  8080\r\n");
	printf("  Image: %dx%d RGB565\r\n", IMG_WIDTH, IMG_HEIGHT);
	printf("========================================\r\n");

	/* 3. Show status on LCD */
	sprintf(status_line1, "WiFi: %s", WIFI_SSID);
	sprintf(status_line2, "IP: %s:8080", g_ip_str);
	sprintf(status_line3, "Waiting for PC...");

	LCD_Clear(WHITE);
	LCD_ShowString(10, 10,  tftlcd_data.width, tftlcd_data.height, 16,
		       (u8 *)status_line1);
	LCD_ShowString(10, 40,  tftlcd_data.width, tftlcd_data.height, 16,
		       (u8 *)status_line2);
	LCD_ShowString(10, 100, tftlcd_data.width, tftlcd_data.height, 12,
		       (u8 *)status_line3);

	printf("\r\n[READY] Waiting for client connection...\r\n");

	/* 4. Main loop: receive +IPD data and display frames */
	while (1) {
		char *ipd;
		u16 data_len;
		char *data_ptr;
		u16 remaining;
		u16 x_offset;
		u16 y_offset;

		if (strEsp8266_Fram_Record.InfBit.FramFinishFlag) {
			strEsp8266_Fram_Record.Data_RX_BUF[
			    strEsp8266_Fram_Record.InfBit.FramLength] = '\0';

			/* Parse +IPD,<link>,<len>: */
			ipd = strstr(strEsp8266_Fram_Record.Data_RX_BUF, "+IPD,");
			if (ipd != NULL) {
				/* skip "+IPD," and link_id */
				ipd += 5;
				while (*ipd >= '0' && *ipd <= '9') ipd++;
				if (*ipd == ',') ipd++;

				data_len = 0;
				while (*ipd >= '0' && *ipd <= '9') {
					data_len = data_len * 10 + (*ipd - '0');
					ipd++;
				}

				data_ptr = strchr(ipd, ':');
				if (data_ptr != NULL) {
					data_ptr++;
					remaining = (u16)(strEsp8266_Fram_Record.Data_RX_BUF
					    + strEsp8266_Fram_Record.InfBit.FramLength
					    - data_ptr);

					if (data_len > remaining)
						data_len = remaining;

					if (image_index + data_len <= IMG_SIZE) {
						memcpy(image_buf + image_index,
						       (u8 *)data_ptr, data_len);
						image_index += data_len;
					}

					if (image_index >= IMG_SIZE) {
						image_complete = 1;
						image_index = 0;
					}
				}
			}

			strEsp8266_Fram_Record.InfBit.FramLength = 0;
			strEsp8266_Fram_Record.InfBit.FramFinishFlag = 0;
		}

		/* Display complete frame */
		if (image_complete) {
			image_complete = 0;
			frame_count++;

			x_offset = (tftlcd_data.width - IMG_WIDTH) / 2;
			y_offset = (tftlcd_data.height - IMG_HEIGHT) / 2;

			LCD_ShowPicture(x_offset, y_offset,
				        IMG_WIDTH, IMG_HEIGHT, image_buf);

			sprintf(status_line3, "Frame #%lu", frame_count);
			LCD_ShowString(10, 100, tftlcd_data.width,
				       tftlcd_data.height, 12,
				       (u8 *)status_line3);
		}

		LED1 = !LED1;
		delay_ms(10);
	}
}
