/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file paths_instruction.h
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
#ifndef __NETWORK_H_
#define __NETWORK_H_
/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* Private typedef -----------------------------------------------------------*/

// 指令枚举
typedef enum {
    INSTRUCTION_LEFT = 0,       // 左转
    INSTRUCTION_RIGHT,          // 右转
    INSTRUCTION_LEFT_FRONT,     // 向左前方行驶
	INSTRUCTION_RIGHT_FRONT,    // 向右前方行驶
    INSTRUCTION_STRAIGHT,       // 直行
    INSTRUCTION_UTURN,          // 掉头
    INSTRUCTION_CONTINUE,       // 继续前行
	INSTRUCTION_DESTINATION,    // 到达目的地
    INSTRUCTION_NONE            // 空或无效指令
} INST_CASE;

// 方向枚举
typedef enum {
    ORIENTATION_EAST = 0,       // 东
	ORIENTATION_SOUTH,          // 南
    ORIENTATION_WEST,           // 西
    ORIENTATION_NORTH,          // 北
    ORIENTATION_NORTHEAST,      // 东北
    ORIENTATION_SOUTHEAST,      // 东南
    ORIENTATION_SOUTHWEST,      // 西南
	ORIENTATION_NORTHWEST,      // 西北
    ORIENTATION_NONE            // 空或无效方向
} ORIE_CASE;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

void bike_cloud_recv_data(void);
void bike_cloud_time_request(void);
void bike_path_plan_request(void);
void bike_cloud_upload_data(void);

/**
 * 查看路径是否已经生成完毕
 * @param
 * @return 返回标志
 */
uint8_t check_ride_path_gener_done(void);

/**
 * 清除路径规划完成标志
 * @param
 * @return 返回标志
 */
uint8_t clear_ride_path_done_flag(void);

/**
 * 获取总距离和总时长数据
 * @param
 * @return 
 */
void get_total_ride_info(uint32_t *total_distance, uint32_t *total_seconds);

/**
 * 获取第n个步骤的相关信息
 * @param
 * @return 
 */
void get_step_n_info(uint16_t step_n, char **inst, char **orie, uint32_t *step_distance);

/**
 * 获取共有步骤数量
 * @param
 * @return 
 */
int get_step_num(void);

/**
 * 释放路径规划内存
 * @param
 * @return 
 */
void reset_path_memory(void);

INST_CASE parse_instruction(const char* input);

ORIE_CASE parse_orientation(const char* input);

float parse_orientation_to_angle(const char* input);

/* Private functions ---------------------------------------------------------*/
#endif /* __NETWORK_H_ */
