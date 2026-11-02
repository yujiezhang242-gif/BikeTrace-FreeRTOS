/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_key.h
 * 
 * @par dependencies 
 * - bsp_key.h
 * - stdio.h
 * - stdint.h
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


#ifndef __BSP_KEY_H_
#define __BSP_KEY_H_

//******************************** Includes *********************************//

#include "main.h"

//******************************** Includes *********************************//


//******************************** Defines **********************************//
/*  按键类型宏定义                   */
#define KEY_GO_STOP_TYPE        (0X11)
#define KEY_SWITCH_SHOW_TYPE    (0X22)

/*  按键状态宏定义                   */
#define KEY_NOT_PRESS     		(0X00)
#define KEY_SHORT_PRESS         (0X01)
#define KEY_LONG_PRESS          (0X81)

/*  按键按下时长宏定义               */
#define KEY_DELAY_TIME       	 (20)
#define KEY_LONGPRESS_TIME   	 (600)

/*  按键状态宏定义                   */
#define RELEASE   (1)
#define PRESSED   (0)

/*  按键状态枚举                     */
typedef enum
{
	KEY_RELEASE = 0,    /* 按键释放 */
	KEY_CONFIRM,        /* 按键按下 */
	KEY_SHORTPRESS,     /* 按键短按 */
	KEY_LONGPRESS       /* 按键长按 */
}key_press_state_t;


/*  函数返回状态枚举                */
typedef enum
{
	KEY_OK = 0,            /* 按键获取成功 */
	KEY_ERROR,             /* 按键获取错误 */
	KEY_ERRORTIMEOUT,      /* 按键获取超时 */
} key_func_state_t;


/*  按键信息结构体                  */
typedef struct
{
	key_press_state_t  keyState;   /* 记录按键状态 */
	uint64_t        lastSysTime;   /* 记录上次时间 */
} key_info_t;

typedef struct
{
	key_info_t info;
	uint8_t (*getkeyStatus)(void);
	uint8_t keyFuncValue;
} bsp_key_driver_t;

extern bsp_key_driver_t key_on_off;
extern bsp_key_driver_t key_switch;
//******************************** Defines **********************************//


/**
***********************************************************
* @brief 获取按键码值
* @param
* @return 短按0x01, 长按0x81
***********************************************************
*/
key_func_state_t GetKeyVal(uint8_t key_type, uint8_t *key_val);




#endif
