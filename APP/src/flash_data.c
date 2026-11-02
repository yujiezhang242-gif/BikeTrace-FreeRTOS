/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file flash_data.c
 * 
 * @par dependencies 
 * - flash_data.h
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
#include "flash_data.h"
#include "flash_index.h"
#include "utils.h"
#include "bike.h"

#include "w25qxx.h"
#include "OLED.h"
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define ITEM_CNT_IN_RECORD 13               /**< 一条记录有几个数据项 */
#define RECORD_SIZE 256                     /**< 一条记录的大小 */

/* Private typedef -----------------------------------------------------------*/
#pragma pack(push, 1)                       /**< 强制结构体的成员紧密排列 */
struct riding_item {                        /**< 18字节 */
    double      lon_deg;                    /**< 实时经度的原始数据，单位度分 */
    double      lat_deg;                    /**< 实时纬度的原始数据，单位度分 */
    uint16_t    speed_kmh;                  /**< 实时速度 */
};
#pragma pack(pop)                           /**< 强制结构体的成员紧密排列 */

struct riding_status {                      /**< 20字节 */
    uint32_t    timestamp;                  /**< UNIX时间戳 */
    uint16_t    max_speed_kmh;              /**< 最大时速 */
    uint16_t    average_speed_kmh;          /**< 平均时速 */
    float       kcal;                       /**< 平均时速 */
    uint16_t    average_humi;               /**< 平均湿度 */
    int16_t     average_temp;               /**< 平均温度 */
    uint16_t    total_distance_km;          /**< 骑行距离，单位：公里 */
    uint8_t     reserved[1];                /**< 保留1字节 */
    uint8_t     write_flag;                 /**< 已写入标记位 0xA5 */
};


/* Private variables ---------------------------------------------------------*/

/**> 是否是写入第一条数据项，如果是，则表示是刚刚启动骑行 */
static uint8_t write_first_item = 1;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  设置第一次写入数据标记位
  * @param  无
  * @retval 无
  */
void set_first_write_flag(void)
{
	/**> 骑行完毕应当将 write_first_item 置1，为下一次骑行做准备 */
    write_first_item = 1;
}

/**> 获取当前骑行状态信息，并将它们转成相应的存储格式 */
static void get_riding_status(struct riding_status* status)
{
    status->write_flag = FLASH_WRITE_FLAG;
    status->reserved[0] = 0;
    status->total_distance_km = float_to_uint16(bike_get_distance_km());
    status->timestamp = time_str_to_timestamp(bike_get_time_str());
    status->max_speed_kmh = float_to_uint16(bike_get_max_speed_kmh());
    status->average_speed_kmh = float_to_uint16(bike_get_average_speed_kmh());
    status->kcal = bike_get_kcal();
    status->average_humi = float_to_uint16(bike_get_average_humi());
    status->average_temp = float_to_int16(bike_get_average_temp());
}

/*
 * 写一条记录到flash
 * 13个 item + 1个status= 1条 record
*/
void write_record_to_flash(struct riding_item items[ITEM_CNT_IN_RECORD], struct riding_status* status) 
{
    int i;
    uint8_t record_buf[RECORD_SIZE] = { 0 };
    uint8_t* ptr = record_buf;
    /* 当前写入flash的页地址 */
    static uint32_t current_page_addr;
	static uint8_t excute_flag = 1;

    // 复制 13 个 riding_item 到缓冲区
    for (i = 0; i < ITEM_CNT_IN_RECORD; i++) {
        memcpy(ptr, &items[i], sizeof(struct riding_item));
        ptr += sizeof(struct riding_item);
    }

    // 两个保留字节
    ptr += 2;

    // 复制 riding_status 到缓冲区
    memcpy(ptr, status, sizeof(struct riding_status));
	
	/* ********** add code ************ */
	
	if (get_restore_flag_status() == IS_RESTORE && excute_flag)
	{
		excute_flag = 0;
		/* 跳过写入索引以及寻找下一个表地址的执行 */
		write_first_item = 0;
		/* 修改当前写入页地址 */
		current_page_addr = get_last_ride_record_addr() + RECORD_SIZE;
		/* 可以执行修改下一个索引的函数 */
		excute_set_next_index_fuc();
		/* 恢复正常状态 */
		clear_restore_flag_status();
	}
	
	/* ********** add code ************ */
    
    if(write_first_item) {
        current_page_addr = get_next_table_addr();
        printf("item start page addr:0x%x\r\n", current_page_addr);
    }

    // 判断是否需要擦除扇区
    if (current_page_addr % 4096 == 0) {
        printf("erase addr 0x%x\r\n", current_page_addr);
        BSP_W25Qx_Erase_Block(current_page_addr);
    }
    printf("write items to addr 0x%x\r\n", current_page_addr);

    // 写入缓冲区到 Flash 页
	BSP_W25Qx_Write(record_buf, current_page_addr, sizeof(record_buf));

    current_page_addr += RECORD_SIZE;

    if(write_first_item) {
        write_first_item = 0;
		// 写入第一个record完毕，写入索引（保证索引信息不丢失）
        write_index();
    }
}

/**
  * @brief  保存一个数据项到记录中
  * @param  lat 纬度
  * @param  lon 经度
  * @param  lspeed_kmh 速度
  * @param  running_state 运行状态，如果是停止状态，则需要立即写入flash
  * @retval 无
  */
void save_item(char* lat, char* lon, float speed_kmh, bike_status_t running_state)
{
    struct riding_status status;
    struct riding_item item;
    /* 保存经纬速度的数组 */
    static struct riding_item items[ITEM_CNT_IN_RECORD] = {0};
    static uint8_t items_idx = 0;
	static uint16_t save_record_cnt = 0;

    sscanf(lat, "%lf", &item.lat_deg);
    sscanf(lon, "%lf", &item.lon_deg);
    item.speed_kmh = float_to_uint16(speed_kmh);

    printf("idx:%d, lat: %lf, lng: %lf, speed: %u\r\n", items_idx, 
        item.lat_deg, item.lon_deg, item.speed_kmh);

    memcpy(&items[items_idx++], &item, sizeof(struct riding_item));

    /* 共写入13条，234字节 */
    if (items_idx == ITEM_CNT_IN_RECORD || running_state == STOP) {
        get_riding_status(&status);
        write_record_to_flash(items, &status);
		
		/* ***** add code ***** */
		save_record_cnt++;
		write_ride_record_count_to_at24(&save_record_cnt);
		if (running_state == STOP)
		{
			/* 延迟一段时间再写入AT24 */
			HAL_Delay(20);
			write_write_flag_to_at24();
		}
		
        /* 清空已经写入的全部经纬速度 */
        memset(items, 0, sizeof(items));
        items_idx = 0;
    }
}

void print_status(int n, struct riding_status* status)
{
    char time_str[32] = {0};
    uint32_t start_ts = get_table_n_created_timestamp(n);
    printf("=====================================================\r\n");
    printf("The [%dth] Riding Status Summary:\r\n", n);
    printf("Start Time: %s\r\n", timestamp_to_time_str(start_ts, time_str));
    printf("End Time: %s\r\n", timestamp_to_time_str(status->timestamp, time_str));
	
	/* TODO: Total Time如果是断电后重新开始的话是不对的，需要重新考虑一下如何计算 */
    printf("Total Time: %s\r\n", format_seconds(status->timestamp - start_ts, time_str));
    printf("Total Distance: %.02f km\r\n", uint16_to_float(status->total_distance_km));
    printf("Max Speed: %.02f km/h\r\n", uint16_to_float(status->max_speed_kmh));
    printf("Average Speed: %.02f km/h\r\n", uint16_to_float(status->average_speed_kmh));
    printf("Calorie: %.02f KCAL\r\n", status->kcal);
    printf("Average Humidity: %.02f%%\r\n", uint16_to_float(status->average_humi));
    printf("Average Temperature: %.02f C\r\n", int16_to_float(status->average_temp));
    printf("=====================================================\r\n");
}

void print_item(struct riding_item* item)
{
    double              lat_dec;       // 10进制，度
    double              lon_dec;
    double              gcj_lat;       // 火星坐标
    double              gcj_lon;

    lat_dec = convert_to_decimal_degrees(item->lat_deg);
    lon_dec = convert_to_decimal_degrees(item->lon_deg);

    gps_to_gcj02(lat_dec, lon_dec, &gcj_lat, &gcj_lon);

    printf("{ lat: %.16lf, lng: %.16lf, speed: %.02f },\r\n", 
        gcj_lat, gcj_lon, uint16_to_float(item->speed_kmh));
}

/**
  * @brief  打印第n个表的内容，即输出第n次骑行数据
  * @param  n 表编号，1~16，对应索引下标0~15
  * @retval 无
  */
void print_table_n(int n)
{
	uint8_t             record_buf[RECORD_SIZE] = {0};
    uint8_t             zero_array[18] = {0};
    int                 i;
    int                 is_valid_data = 0;
    uint8_t             npage = 0;
    uint32_t            start = get_table_n_addr(n);
    uint32_t            addr;
    struct riding_item  *item;
    struct riding_status last_status;

    OLED_Clear_Part(2, 1, 16);
    OLED_ShowString(2, 1, " data exproting");

    while (1) {
        memset(record_buf, 0, sizeof(record_buf));
        
        addr = start + npage * RECORD_SIZE;

		BSP_W25Qx_Read(record_buf, addr, RECORD_SIZE);

        if(record_buf[255] == FLASH_WRITE_FLAG) {
            if(is_valid_data == 0) {
                // 输出一次开头
                is_valid_data = 1;
                printf("var data = [\r\n");
            }
            
            memcpy(&last_status, &record_buf[236], sizeof(struct riding_status));
            
            item = (struct riding_item*)record_buf;
            
            OLED_Clear_Part(3, 1, 16);

            for(i = 0; i < ITEM_CNT_IN_RECORD; i++, item++) {
                /* 如果3个变量均为0，则是数据结尾，不再循环输出 */
                if(memcmp(item, zero_array, sizeof(struct riding_item)) == 0)
                    break;
                print_item(item);
                // 为更好和用户交互，屏幕显示类似进度条的读出效果
                OLED_ShowString(3, i + 2, ".");
            }
        } else{
            if(is_valid_data == 1) {
                // 输出一次结尾，如果一条有效数据都没有发现，则什么都不输出
                printf("];\r\n");
                print_status(n, &last_status);
            }
            break;
        }

        npage++;
    }
    
    OLED_Clear_Part(2, 1, 16);
    OLED_Clear_Part(3, 1, 16);
}


/**
  * @brief  初始化flash
  * @param  无
  * @retval 无
  */
void init_flash(void)
{
    uint8_t MID;

     /* 初始化SPI */
    
    BSP_W25Qx_Init();
    BSP_W25Qx_Read_ID(&MID);

    printf("MID:0x%X\r\n", MID);
    /* 这里只校验了制造商ID，严格来讲可以同时校验设备ID */    
    if (MID != 0xEF) {
        printf("SPI Flash W25QXX test fail\r\n");
        //while(1) ;
    }
    /* 如果之前运行过 Plus 版本，此处需要先擦除一次全片flash，因为标记位是相同的 */
    //w25qxx_chip_erase();【擦除FLASH可以加一个功能，删除历史记录-可初步先通过串口CMD实现】

    init_index();
}
