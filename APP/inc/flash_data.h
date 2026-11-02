/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file flash_data.h
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
#ifndef __FLASH_DATA_H_
#define __FLASH_DATA_H_
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "bike.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/**
  * @brief  打印第n个表的内容，即输出第n次骑行数据
  * @param  n 表编号，1~16，对应索引下标0~15
  * @retval 无
  */
void print_table_n(int n);
/**
  * @brief  保存一个数据项到记录中
  * @param  lat 纬度
  * @param  lon 经度
  * @param  lspeed_kmh 速度
  * @param  running_state 运行状态，如果是停止状态，则需要立即写入flash
  * @retval 无
  */
void save_item(char* lat, char* lon, float speed_kmh, bike_status_t status);
/**
  * @brief  设置第一次写入数据标记位
  * @param  无
  * @retval 无
  */
void set_first_write_flag(void);
/**
  * @brief  初始化flash
  * @param  无
  * @retval 无
  */
void init_flash(void);

/* Private functions ---------------------------------------------------------*/

#endif /* __FLASH_DATA_H_ */
