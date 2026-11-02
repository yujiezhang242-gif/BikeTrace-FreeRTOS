/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file flash_index.c
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
#ifndef __FLASH_INDEX_H_
#define __FLASH_INDEX_H_
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "bike.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

#define FLASH_WRITE_FLAG               0xA5                /* flash页写入标记 */

/**
  * @brief  设置一张表的产生时间，即本次骑行的开始时间
  * @param  ts UNIX时间戳
  * @retval 无
  */
void set_table_created_timestamp(uint32_t ts);
/**
  * @brief  获取第n个表创建的时间戳，即第n次骑行的开始时间
  * @param  n 表编号，1~16，对应索引下标0~15
  * @retval 表创建的时间戳
  */
uint32_t get_table_n_created_timestamp(int n);
/**
  * @brief  获取第n个表的起始地址
  * @param  n 表编号，1~16，对应索引下标0~15
  * @retval 第n个表的地址
  */
uint32_t get_table_n_addr(int n);
/**
  * @brief  获取准备写入的表的起始地址
  * @param  无
  * @retval 准备写入的表的起始地址
  */
uint32_t get_next_table_addr(void);
/**
  * @brief  获取表数量，即骑行次数
  * @param  无
  * @retval 有多少个表，范围0~16
  */
uint16_t get_table_cnt(void);
/**
  * @brief  获取页索引
  * @param  无
  * @retval 范围0~16
  */
uint16_t get_next_page_index(void);
/**
  * @brief  读取flash中的骑行次数
  * @param  无
  * @retval 无
  */
uint32_t get_ride_times(void);

/**
  * @brief  写入新的索引
  * @param  无
  * @retval 无
  */
void write_index(void);
/**
  * @brief  执行一次设置下一个索引函数
  * @param  无
  * @retval 无
  */
void excute_set_next_index_fuc(void);
/**
  * @brief  读取flash中的索引数据，对索引结构体初始化
  * @param  无
  * @retval 无
  */
void init_index(void);

/* Private functions ---------------------------------------------------------*/

#endif /* __FLASH_INDEX_H_ */
