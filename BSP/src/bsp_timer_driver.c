/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_timer_driver.c
 * 
 * @par dependencies 
 * - bsp_timer_driver.h
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
#include "bsp_timer_driver.h"
#include "tim.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static uint32_t timTick = 0;
static uint8_t timTrigEvenFlag = 0;

/* Private function prototypes -----------------------------------------------*/

static void start(void);
static void stop(void);
static void reset(void);
static uint32_t getTick(void);
static uint8_t getTimFlag(void);
static void clearTimFlag(void);

bsp_timer_driver_t tim =
{
	.start = start,
	.stop = stop,
	.reset = reset,
	.getTick = getTick,
	.getTimFlag = getTimFlag,
	.clearTimFlag = clearTimFlag,
};
	
/* Private functions ---------------------------------------------------------*/

static void start(void)
{
	__HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE); // 清除更新中断标志
	__HAL_TIM_SET_COUNTER(&htim2, 0);              // 清空计数器
	HAL_TIM_Base_Start_IT(&htim2);                 // 启动
}

static void stop(void)
{
	HAL_TIM_Base_Stop(&htim2);
}

static void reset(void)
{
	HAL_TIM_Base_Stop(&htim2);
	timTick = 0;
}
static uint32_t getTick(void)
{
	return timTick;
}

static uint8_t getTimFlag(void)
{
	return timTrigEvenFlag;
}

static void clearTimFlag(void)
{
	timTrigEvenFlag = 0;
}

/**
***********************************************************
* @brief 定时中断函数
* @param  
* @return 
***********************************************************
*/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) 
{
	if (htim->Instance == TIM2) 
	{
		timTick++;
		// 触发事件
		timTrigEvenFlag = 1;
	}
}


