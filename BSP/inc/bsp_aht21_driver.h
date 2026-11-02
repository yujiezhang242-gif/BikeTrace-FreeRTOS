/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_aht21_handle.h
 * 
 * @par dependencies 
 * - main.h
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
 #ifndef __BSP_AHT21_DRIVER_H__
 #define __BSP_AHT21_DRIVER_H__
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private typedef -----------------------------------------------------------*/

typedef struct
{
	void (*init)(void);
	int (*parse_humi_temp)(void);
	float (*getHumiVal)(void);
	float (*getTempVal)(void);
} bsp_aht21_driver_t;

extern bsp_aht21_driver_t aht21;

/* Private define ------------------------------------------------------------*/

/* AHT10µÿ÷∑ */
#define AHT21_WRITE_BIT                                          (uint8_t)(0x00)
#define AHT21_READ_BIT                                           (uint8_t)(0x00)
#define AHT21_ADDRESS  									    (uint8_t)(0x38 << 1)
#define AHT21_CHECK_BYTE                                         (uint8_t)(0x18)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
#endif //__BSP_AHT21_DRIVER_H__
