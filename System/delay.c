/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file delay.c
 * 
 * @par dependencies 
 * - delay.h
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
#include "delay.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
void Delay_Us(uint32_t us)
{
//	/* 系统主频为100MHz, 100计数值表示1us */
//	SysTick->LOAD = us * 100;
//	/* 写入VAL值时，无论何值VAL都会被清零 */
//	SysTick->VAL = 0;
//	/* 打开滴答定时器控制标志位 */
//	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
//	/* 等待标志位置1 */
//	while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));
//	/* 关闭滴答定时器控制标志位 */
//	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	

    // 72MHz优化参数：每微秒需要约72个周期
    // 循环开销≈3周期/次 (需根据实际芯片调整)
    volatile uint32_t cycles = us * 12;  // 72 ÷ 3 = 24
    
    while (cycles--) {
        __ASM volatile ("nop");  // 防止编译器优化
    }
	
}

void Delay_Ms(uint32_t ms)
{
	while (ms--)
	{
		Delay_Us(1000);
	}
}
