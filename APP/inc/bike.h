/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bike.h
 * 
 * @par dependencies 
 * - bike.h
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
#ifndef __BIKE_H_
#define __BIKE_H_
 
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "restore.h"


/* Private typedef -----------------------------------------------------------*/

typedef enum
{
	IDLE = 0,
	RIDE,
	STOP,
} bike_status_t;

typedef enum
{
	SCREEN_IDEL = 0,
	SCREEN_PATH,
	SCREEN_1,
	SCREEN_2,
} screen_seq_t;

void test(char *time, float distance_km);
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* 系列 - 获取骑行数据 */
float bike_get_distance_km(void);
char* bike_get_time_str(void);
float bike_get_max_speed_kmh(void);
float bike_get_average_speed_kmh(void);
float bike_get_kcal(void);
float bike_get_average_humi(void);
float bike_get_average_temp(void);
float bike_get_temp(void);
float bike_get_humi(void);
float bike_get_speed_kmh(void);
uint32_t bike_get_total_time_sec(void);
bike_status_t bike_get_status(void);
// 获取本地时间
uint32_t bike_get_local_time_val(void);
/* *************** add code ***************** */
/* 传递骑行一系列参数 */
void bike_set_distance_km(uint16_t total_distance_km);
void bike_set_total_time_sec(uint32_t last_timestamp);
void bike_set_max_speed_kmh(uint16_t max_speed_kmh);
void bike_set_average_speed_kmh(uint16_t average_speed_kmh);
void bike_set_kcal(float kcal);
void bike_set_average_humi(uint16_t average_humi);
void bike_set_average_temp(int16_t average_temp);
// 设置本地时间
void bike_set_local_time_val(uint32_t time_set_val);
	
/* *************** add code ***************** */

/* 当前空闲，切换至骑行状态 */
void bike_switch_ride_status_handle(Restore_Flag restore_flag);

/* 骑行数据输出请求 */
void bike_data_exprot_request(char* cmd);

/* 骑行数据输出请求 */
void bike_data_list_request(void);

/* 骑行处理任务 */
void bike_handle(void);

/* 骑行初始化 */
void bike_init(void);

// 获取箭头宏定义
int get_arrow_macro(float rel_angle);

/* Private functions ---------------------------------------------------------*/

#endif /* __BIKE_H_ */
