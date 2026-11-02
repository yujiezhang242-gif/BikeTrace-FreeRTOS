/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file restore.h
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
#ifndef __RESTORE_H_
#define __RESTORE_H_
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private typedef -----------------------------------------------------------*/

typedef enum
{
	NOT_RESTORE = 0,
	IS_RESTORE,
} Restore_Flag;


/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

void clear_restore_flag_status(void);

Restore_Flag get_restore_flag_status(void);
/**
  * @brief  将索引数据的页索引写入，用于开机校验
  * @param  无
  * @retval 0 表示失败，1表示成功
  */
uint8_t write_page_index_to_at24(uint8_t* page_index);
/**
  * @brief  骑行结束标志位写入，骑行开始前进行清除
  * @param  无
  * @retval 0 表示失败，1表示成功
  */
uint8_t write_write_flag_to_at24(void);

/**
  * @brief  骑行结束标志位清除
  * @param  无
  * @retval 0 表示失败，1表示成功
  */
uint8_t clear_write_flag(void);
/**
  * @brief  写入骑行record次数
  * @param  无
  * @retval 0 表示失败，1表示成功
  */
uint8_t write_ride_record_count_to_at24(uint16_t* ride_record_count);
/**
  * @brief  骑行record次数清除
  * @param  无
  * @retval 0 表示失败，1表示成功
  */
uint8_t clear_ride_record_count(void);

/**
  * @brief  开机判断是否上次骑行是否正常结束
  * @param  无
  * @retval 0 - 正常， 1 - 异常
  */
Restore_Flag judge_last_ride_finished_normal(void);

/**
  * @brief  获取上次骑行的最后记录地址
  * @param  无
  * @retval 无
  */
uint32_t get_last_ride_record_addr(void);

/**
  * @brief  恢复上次异常骑行状态，需要GPS有效后调用
  * @param  无
  * @retval 无
  */
void restore_last_ride_status(void);

void print_restore_status_info(void);
/* Private functions ---------------------------------------------------------*/
#endif /* __RESTORE_H_ */
