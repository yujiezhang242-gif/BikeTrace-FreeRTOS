/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_key_driver.c
 * 
 * @par dependencies 
 * - bsp_key_driver.h
 * 
 *
 * 
 * @author Jack | R&D Dept. | EternalChip 立芯嵌入式
 * 
 * @brief Provide the HAL APIs of AHT21 and corresponding opetions.
 * 
 * Processing flow:
 * 
 * call directly.
 * 
 * @version V1.0 2025-2-24
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/

//******************************** Includes *********************************//

#include "bsp_key_driver.h"     // 包含自己的寄存器.h文件一定是在最开始
#include "main.h"

//******************************** Includes *********************************//

static uint8_t GetKey1Val(void);
static uint8_t GetKey2Val(void);

/* PC14 */
bsp_key_driver_t key_on_off = 
{
	.info = {KEY_RELEASE, 0},
	.getkeyStatus = GetKey1Val,
	.keyFuncValue = 0,
};

/* PB0 */
bsp_key_driver_t key_switch = 
{
	.info = {KEY_RELEASE, 0},
	.getkeyStatus = GetKey2Val,
	.keyFuncValue = 0,
};

/**
***********************************************************
* @brief 获取按键码值
* @param  传入获取按键码值的参数地址（未按下0x00, 短按0x01, 长按0x81）
* @return 返回函数状态值
***********************************************************
*/
uint8_t GetKey1Val(void)
{
	/* 系统运行时间 */
	uint64_t SysRunTime;
	/* 读取按键电平状态 */
	
	uint8_t ReadKeyPin, key_val;

	ReadKeyPin = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_14);
	
	/* 按键状态机 */
	switch(key_on_off.info.keyState)
	{
		case KEY_RELEASE:
			if(PRESSED == ReadKeyPin)
			{
				key_on_off.info.keyState = KEY_CONFIRM;
				key_on_off.info.lastSysTime = HAL_GetTick();
			}
			else
			{
				key_val = KEY_NOT_PRESS;
			}
			break;
		case KEY_CONFIRM:
			if(PRESSED == ReadKeyPin)
			{
				SysRunTime = HAL_GetTick();
				if(SysRunTime - key_on_off.info.lastSysTime > KEY_DELAY_TIME)
				{
					key_on_off.info.keyState = KEY_SHORTPRESS;
					key_on_off.info.lastSysTime = HAL_GetTick();
				}
			}
			else
			{
				key_on_off.info.keyState = KEY_RELEASE;
			}
			break;
		case KEY_SHORTPRESS:
			if(PRESSED == ReadKeyPin)
			{
				SysRunTime = HAL_GetTick();
				if(SysRunTime - key_on_off.info.lastSysTime > KEY_LONGPRESS_TIME)
				{
					key_on_off.info.keyState = KEY_LONGPRESS;
					key_on_off.info.lastSysTime = HAL_GetTick();
				}
			}
			else
			{
				key_on_off.info.keyState = KEY_RELEASE;
				key_val = KEY_SHORT_PRESS;
				//return KEY_OK;
				
			}
			break;
		case KEY_LONGPRESS:
			if(RELEASE == ReadKeyPin)
			{
				key_on_off.info.keyState = KEY_RELEASE;
				key_val = KEY_LONG_PRESS;
				//return KEY_OK;
			}
			break;
		default:
			key_on_off.info.keyState = KEY_RELEASE;
			break;
	}
	return key_val;
}

uint8_t GetKey2Val(void)
{
	/* 系统运行时间 */
	uint64_t SysRunTime;
	/* 读取按键电平状态 */
	
	uint8_t ReadKeyPin, key_val;

	ReadKeyPin = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);
	
	/* 按键状态机 */
	switch(key_switch.info.keyState)
	{
		case KEY_RELEASE:
			if(PRESSED == ReadKeyPin)
			{
				key_switch.info.keyState = KEY_CONFIRM;
				key_switch.info.lastSysTime = HAL_GetTick();
			}
			else
			{
				key_val = KEY_NOT_PRESS;
			}
			break;
		case KEY_CONFIRM:
			if(PRESSED == ReadKeyPin)
			{
				SysRunTime = HAL_GetTick();
				if(SysRunTime - key_switch.info.lastSysTime > KEY_DELAY_TIME)
				{
					key_switch.info.keyState = KEY_SHORTPRESS;
					//key_switch.info.lastSysTime = HAL_GetTick();
				}
			}
			else
			{
				key_switch.info.keyState = KEY_RELEASE;
			}
			break;
		case KEY_SHORTPRESS:
			if(PRESSED == ReadKeyPin)
			{
				SysRunTime = HAL_GetTick();
				if(SysRunTime - key_switch.info.lastSysTime > KEY_LONGPRESS_TIME)
				{
					key_switch.info.keyState = KEY_LONGPRESS;
					//key_switch.info.lastSysTime = HAL_GetTick();
				}
			}
			else
			{
				key_switch.info.keyState = KEY_RELEASE;
				key_val = KEY_SHORT_PRESS;
				//return KEY_OK;
				
			}
			break;
		case KEY_LONGPRESS:
			if(RELEASE == ReadKeyPin)
			{
				key_switch.info.keyState = KEY_RELEASE;
				key_val = KEY_LONG_PRESS;
				//return KEY_OK;
			}
			break;
		default:
			key_switch.info.keyState = KEY_RELEASE;
			break;
	}
	return key_val;
}


