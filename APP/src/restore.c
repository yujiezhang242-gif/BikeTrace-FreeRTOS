/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file restore.c
 * 
 * @par dependencies 
 * - restore.h
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
#include "restore.h"

#include "flash_index.h"
#include "utils.h"

#include "bsp_at24c02_driver.h"
#include "w25qxx.h"

/* Private typedef -----------------------------------------------------------*/

typedef struct 
{
	uint8_t page_index;
	uint8_t write_flag;
	uint16_t ride_record_count;
} restore_param_t;

typedef struct 
{                
	uint8_t     reserved_byte[256 - 20];    /**< 占用 */
	/**< 20字节 */
    uint32_t    timestamp;                  /**< UNIX时间戳 */
    uint16_t    max_speed_kmh;              /**< 最大时速 */
    uint16_t    average_speed_kmh;          /**< 平均时速 */
    float       kcal;                       /**< 平均时速 */
    uint16_t    average_humi;               /**< 平均湿度 */
    int16_t     average_temp;               /**< 平均温度 */
    uint16_t    total_distance_km;          /**< 骑行距离，单位：公里 */
    uint8_t     reserved[1];                /**< 保留1字节 */
    uint8_t     write_flag;                 /**< 已写入标记位 0xA5 */
} restore_riding_status_t;

static restore_riding_status_t g_restore_status;

static Restore_Flag g_restore_flag = NOT_RESTORE;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

void clear_restore_flag_status(void)
{
	g_restore_flag = NOT_RESTORE;
}

Restore_Flag get_restore_flag_status(void)
{
	return g_restore_flag;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  将索引数据的页索引写入，用于开机校验
  * @param  无
  * @retval 0 表示失败，1表示成功
  */
uint8_t write_page_index_to_at24(uint8_t* page_index)
{
	return at24c02.writeBytes(page_index, PAGE_INDEX_ADDR, 1);
}

/**
  * @brief  骑行结束标志位写入，骑行开始前进行清除
  * @param  无
  * @retval 0 表示失败，1表示成功
  */
uint8_t write_write_flag_to_at24(void)
{
	uint8_t write_flag = RIDE_DONE_FLAG;
	return at24c02.writeBytes(&write_flag, WRITE_FLAG_ADDR, 1);
}

/**
  * @brief  骑行结束标志位清除
  * @param  无
  * @retval 0 表示失败，1表示成功
  */
uint8_t clear_write_flag(void)
{
	uint8_t earse_flag = ERASE_FLAG;
	//return at24c02.writeBytes((uint8_t *)ERASE_FLAG, WRITE_FLAG_ADDR, 1);
	return at24c02.writeBytes(&earse_flag, WRITE_FLAG_ADDR, 1);
}

/**
  * @brief  写入骑行record次数
  * @param  无
  * @retval 0 表示失败，1表示成功
  */
uint8_t write_ride_record_count_to_at24(uint16_t* ride_record_count)
{
	return at24c02.writeBytes((uint8_t *)ride_record_count, RIDE_RECORE_COUNT_ADDR, 2);
}

/**
  * @brief  骑行record次数清除
  * @param  无
  * @retval 0 表示失败，1表示成功
  */
uint8_t clear_ride_record_count(void)
{
	uint16_t earse_data = 0xffff;
	
	return at24c02.writeBytes((uint8_t *)&earse_data, RIDE_RECORE_COUNT_ADDR, 2);
}

/**
  * @brief  开机判断是否上次骑行是否正常结束
  * @param  无
  * @retval 0 - 正常， 1 - 异常
  */
Restore_Flag judge_last_ride_finished_normal(void)
{
	uint8_t page_index;
	uint8_t read_page_index;
	uint8_t ride_done_flag;
	
	/* 1.check pageindex and at24 if march */
	page_index = (uint8_t)get_next_page_index();
	at24c02.readBytes(&read_page_index, PAGE_INDEX_ADDR, 1);
	
	
	if (page_index == read_page_index)
	{
		/* 1.1.check if ride_done_flag write */
		at24c02.readBytes(&ride_done_flag, WRITE_FLAG_ADDR, 1);
		printf("ride_done_flag = [%x]\r\n", ride_done_flag);
		
		if (ride_done_flag != RIDE_DONE_FLAG)
		{
			g_restore_flag = IS_RESTORE;
			printf("last ride is not normal end!\r\n");
		}
	}
	else
	{
		printf("page_index != read_page_index\r\n");
	}
	
	return g_restore_flag;
}

/**
  * @brief  获取上次骑行的最后记录地址（返回的是已写入的最后一页地址）
  * @param  无
  * @retval 无
  */
uint32_t get_last_ride_record_addr(void)
{
	uint16_t last_ride_record_cnt;
	uint32_t last_table_addr;
	
	/* read the record count to restore w25q64 record */
	at24c02.readBytes((uint8_t *)&last_ride_record_cnt, RIDE_RECORE_COUNT_ADDR, 2);
	
	//printf("last_ride_record_cnt = [%d]\r\n", last_ride_record_cnt);
	
	if (last_ride_record_cnt == 0xffff)
	{
		return 0xffffffff;
	}
	
	/* read the last table cnt (must call before the func of 'set_next_index' */
	last_table_addr = get_next_table_addr();
	
	printf("last_table_addr = [%x]\r\n", last_table_addr);
	
	return (last_table_addr + (last_ride_record_cnt - 1) * PAGE_SIZE);
}

/**
  * @brief  恢复上次异常骑行状态，需要GPS有效后调用
  * @param  无
  * @retval 无
  */
void restore_last_ride_status(void)
{
	uint32_t last_recode_add = get_last_ride_record_addr();
	
	/* 参数防御 */
	if (last_recode_add == 0xffffffff)
		return;
	
	/* restore status data from w25q64 */
	BSP_W25Qx_Read((uint8_t*)&g_restore_status, last_recode_add, \
										sizeof(restore_riding_status_t));
	
	/* 传递恢复骑行的一系列参数 */
    bike_set_distance_km(g_restore_status.total_distance_km);
    bike_set_total_time_sec(g_restore_status.timestamp);
    bike_set_max_speed_kmh(g_restore_status.max_speed_kmh);
    bike_set_average_speed_kmh(g_restore_status.average_speed_kmh);
    bike_set_kcal(g_restore_status.kcal);
    bike_set_average_humi(g_restore_status.average_humi);
    bike_set_average_temp(g_restore_status.average_temp);
	
	/* switch status, include ride_status and screen_index */
	bike_switch_ride_status_handle(IS_RESTORE);
}

/* 用于提供串口调试打印上一次异常中断记录的最后一页骑行数据 */
void print_restore_status_info(void)
{
	uint32_t last_recode_addr = get_last_ride_record_addr();
	
	if (last_recode_addr == 0xffffffff)
	{
		printf("last_record_addr err!\r\n");
		return;
	}
	
	char time_str[32];
	
	/* restore status data from w25q64 */
	BSP_W25Qx_Read((uint8_t*)&g_restore_status, last_recode_addr, \
										sizeof(restore_riding_status_t));
	
	uint16_t current_table = get_table_cnt();
    uint32_t create_timestamp = get_table_n_created_timestamp(current_table);
	
	printf("============== restore status info ====================\r\n");
	
    printf("Get TimeSatamp: %d\r\n", g_restore_status.timestamp);
	printf("Total Second: %d\r\n", g_restore_status.timestamp - create_timestamp);
	printf("Total Time: %s\r\n", format_seconds(g_restore_status.timestamp - create_timestamp, time_str));
    printf("Total Distance: %.02f km\r\n", uint16_to_float(g_restore_status.total_distance_km));
    printf("Max Speed: %.02f km/h\r\n", uint16_to_float(g_restore_status.max_speed_kmh));
    printf("Average Speed: %.02f km/h\r\n", uint16_to_float(g_restore_status.average_speed_kmh));
    printf("Calorie: %.02f KCAL\r\n", g_restore_status.kcal);
    printf("Average Humidity: %.02f%%\r\n", uint16_to_float(g_restore_status.average_humi));
    printf("Average Temperature: %.02f C\r\n", int16_to_float(g_restore_status.average_temp));
	
	printf("============== restore status info ====================\r\n");
}

