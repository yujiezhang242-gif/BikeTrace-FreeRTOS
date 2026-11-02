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
 #ifndef __BSP_HMC5883L_DRIVER_H__
 #define __BSP_HMC5883L_DRIVER_H__
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private typedef -----------------------------------------------------------*/

typedef struct
{
	void (*init)(void);
	float (*getAngle)(void);
	void (*calibration)(void);
} bsp_hmc5883l_driver_t;

extern bsp_hmc5883l_driver_t hmc5883l;

uint8_t HMC5983_Status(void);

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
#endif //__BSP_HMC5883L_DRIVER_H__
