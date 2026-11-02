/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_uart_driver.h
 * 
 * @par dependencies 
 * - usart.h
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
 #ifndef __BSP_GPS_DRIVER_H__
 #define __BSP_GPS_DRIVER_H__
/* Includes ------------------------------------------------------------------*/
#include "main.h"


/* Private typedef -----------------------------------------------------------*/

typedef enum
{
	GPRMC_NAME = 0,
	UTC_TIME,
	VALID_VAL,
	LATITUDE_VAL,
	LATITUDE_TYPE,
	LONGITUDE_VAL,
	LONGITUDE_TYPE,
	SPEED_VAL,
	EMPTY1,
	UTC_DATE,
	EMPTY2,
	MODE,
	CHECKSUM,
} gprmc_index;

typedef struct
{
	void (*rec_gps_data)(uint8_t data);
	void (*parse_gprmc_data)(void);
	uint8_t (*get_gprmc_flag)(void);
} bsp_gps_driver_t;

typedef struct
{
	bsp_gps_driver_t handle;
	char* (*get_time_str)  (void);
	int   (*get_valid_val)(void);
	char* (*get_latitude)  (void);
	char* (*get_longitude) (void);
	float (*get_speed_kn)  (void);
} bsp_gps_handle_t;

extern bsp_gps_handle_t gps;

/* Private define ------------------------------------------------------------*/

// 检查参数是否合理
#define CHECK_PARAM(arr, param) (param >= (uint8_t)(sizeof(arr)-1) ? (uint8_t)(sizeof(arr)-1) : param)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

#endif /* __BSP_GPS_DRIVER_H__ */

