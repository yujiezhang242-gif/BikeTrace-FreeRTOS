/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file screen.c
 * 
 * @par dependencies 
 * - screen.h
 * 
 * @author 
 * 
 * @brief Functions related to reading and writing in the chip's flash area.
 * 
 * Processing flow:
 * 
 * call directly.
 * 
 * @version V1.0 2024-09-13
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "screen.h"
#include "bsp_gps_driver.h"
#include "bsp_aht21_driver.h"
#include "OLED.h"
#include "OLED_Bmp.h"

#include "bike.h"
#include "utils.h"
#include "flash_index.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

static void screen_init(void);
static void screen_clear(void);
static void screen_welcome_show(char* time, int gps_enable);
static void screen_path_show(char *time, uint32_t remain_m, INST_CASE instcase, \
														uint8_t arrow_index);
static void screen_1_show(char* time, float speed_kmh, float distance_km, \
									float total_seconds, int running_state);
static void screen_2_show(float max_speed_kmh, float average_speed_kmh, \
					float kcal, float temp, float humi, int running_state);

static void screen_show_reloading(void);

app_screen_t screen =
{
    .init = screen_init,
    .clear = screen_clear,
	.idle_show = screen_welcome_show,
	.path_show = screen_path_show,
	.info_1_show = screen_1_show,
    .info_2_show = screen_2_show,
	.reload_show = screen_show_reloading,
};
/* Private functions ---------------------------------------------------------*/
static void screen_show_direction(uint8_t arrow_index);

void screen_clear(void)
{
    OLED_Clear();
}

static void screen_welcome_show(char* time, int gps_enable)
{
	char oled_str[32] = {0};
	
    OLED_ShowString(1, 1, time);
    OLED_ShowString(2, 2, "Welcome to use");
    OLED_ShowString(3, 3, "Guo Guo Bike");
    
    if (!gps_enable) {
        OLED_ShowString(4, 2, "GPS NOT Ready!");
		LED_GPS_OFF();
    } else {
        OLED_Clear_Part(4, 1, 16);
		
		sprintf(oled_str, "Start %dth Ride", get_ride_times()); 
		OLED_ShowString(4, 1, oled_str);
		LED_GPS_ON();
    }
}

static void screen_1_show(char* time, float speed_kmh, float distance_km, \
									float total_seconds, int running_state)
{
    char oled_str[32] = {0};
    char* p;
    
    OLED_ShowString(1, 1, time);

    sprintf(oled_str, "%.02f km/h", speed_kmh);
    OLED_ShowString(2, 1, oled_str);
   
    sprintf(oled_str, "%.02f km", distance_km);
    OLED_ShowString(3, 1, oled_str);

    format_seconds(total_seconds, oled_str);
    OLED_ShowString(4, 1, oled_str);
    
    p = (running_state == RIDE) ? "go!" : "stop";
    OLED_ShowString(4, 11, p);
}

static void screen_2_show(float max_speed_kmh, float average_speed_kmh, \
					float kcal, float temp, float humi, int running_state)
{
/*
 * 第二屏
 *  - 最大速度，单位km/h，范围 0.0km/h~99.99km/h
 *  - 平均速度，单位km/h，范围 0.0km/h~99.99km/h
 *  - 卡路里，单位KCAL
 *  - 实时温度，单位摄氏度；实时湿度，百分比
 */
    char oled_str[32] = {0};
    char* p;

    sprintf(oled_str, "%.02f km/h", max_speed_kmh);
    OLED_ShowString(1, 1, oled_str);

    sprintf(oled_str, "%.02f km/h", average_speed_kmh);
    OLED_ShowString(2, 1, oled_str);

    sprintf(oled_str, "%.02f KCAL", kcal);
    OLED_ShowString(3, 1, oled_str);

    sprintf(oled_str, "%.02fC %.02f%%", temp, humi);
    OLED_ShowString(4, 1, oled_str);
    
    p = (running_state == RIDE) ? "go!" : "stop";
    OLED_ShowString(1, 13, p);
}

static void screen_path_show(char *time, uint32_t remain_m, INST_CASE instcase, uint8_t arrow_index)
{
	char oled_str[32] = {0};
	
	OLED_ShowString(1, 1, time);
	
	switch(instcase)
	{
		case INSTRUCTION_LEFT:
			OLED_DrawBMP(2,2,50,8,L);
			//printf("L\r\n");
		break;
		
		case INSTRUCTION_RIGHT:
			OLED_DrawBMP(2,2,49,8,R);
			//printf("R\r\n");
		break;
		
		case INSTRUCTION_RIGHT_FRONT:
			OLED_DrawBMP(2,2,49,8,SR);
			//printf("SR\r\n");
		break;
		
		case INSTRUCTION_LEFT_FRONT:
			OLED_DrawBMP(2,2,50,8,SL);
			//printf("SL\r\n");
		break;
		
		case INSTRUCTION_STRAIGHT:
			OLED_DrawBMP(2,2,50,8,S);
			//printf("S\r\n");
		break;
		
		case INSTRUCTION_UTURN:
			OLED_DrawBMP(2,2,50,8,TT);
			//printf("TT\r\n");
		break;
		
		case INSTRUCTION_CONTINUE:
			OLED_DrawBMP(2,2,50,8,S);
			//printf("S\r\n");
		break;
		
		case INSTRUCTION_DESTINATION:
			OLED_DrawBMP(2,2,50,8,DEST);
		break;
			OLED_DrawBMP(2,2,50,8,S);
		case INSTRUCTION_NONE:
			
		break;
	}
	
	sprintf(oled_str, "%4d m", remain_m);
    OLED_ShowString(3, 9, oled_str);
	
	screen_show_direction(arrow_index);
	
}

static void screen_show_direction(uint8_t arrow_index)
{
	switch (arrow_index) {
		case ARROW_UP: 
			OLED_ShowString(4, 15, " ^");  // 上
			break;
		case ARROW_UP_RIGHT: 
			OLED_ShowString(4, 15, "/^");  // 右上（近似）
			break;
		case ARROW_RIGHT: 
			OLED_ShowString(4, 15, " >");  // 右
			break;
		case ARROW_DOWN_RIGHT: 
			OLED_ShowString(4, 15, "\v"); // 右下（近似，反斜杠）
			break;
		case ARROW_DOWN: 
			OLED_ShowString(4, 15, " v");  // 下
			break;
		case ARROW_DOWN_LEFT: 
			OLED_ShowString(4, 15, "v/");  // 左下（近似）
			break;
		case ARROW_LEFT: 
			OLED_ShowString(4, 15, " <");  // 左
			break;
		case ARROW_UP_LEFT: 
			OLED_ShowString(4, 15, "^\\"); // 左上（近似）
			break;
		default: 
			//OLED_ShowString(4, 16, "?"); 
		break;
	}
}

static void screen_show_reloading(void)
{
	static uint8_t i = 0;
	//OLED_Clear();
	OLED_ShowString(2, 2, "Reloading");

	OLED_ShowString(2, 11+i, ".");
	i++;
	if(i == 4)
	{
		i = 0;
		HAL_Delay(200);
		OLED_ShowString(2, 11, "    ");
	}

}

void screen_init(void)
{
    /* 初始化OLED */
    OLED_Init();
}
