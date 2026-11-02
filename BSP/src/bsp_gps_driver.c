/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_uart_driver.c
 * 
 * @par dependencies 
 * - bsp_uart_driver.h
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
#include "bsp_gps_driver.h"
#include "bsp_uart_driver.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

#define REC_GPS_DATA_BUFLEN     (256)
#define PARSE_GPS_DATA_BUFLEN   (128)
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static uint8_t     rec_gps_data[REC_GPS_DATA_BUFLEN];
static uint8_t parse_gps_data[PARSE_GPS_DATA_BUFLEN];

// 获取的gprmc长度（实际可能没效果）
static uint16_t g_gprmc_len = 0;

// 获取gprmc语句标志，用于检测开启解析
static uint8_t g_get_gprmc_flag = 0;

typedef struct
{
	char  time_str[18];
	int   valid_val;
	float speed_kn;
	char  latitude[12];
	char  latitude_type[2];
	char  longitude[13];
	char  longitude_type[2];
} gps_param_t;

static gps_param_t g_gps_param_t;

/* Private function prototypes -----------------------------------------------*/
static void rec_gps_data_to_get_gprmc(uint8_t data);
static void parse_gps_gprmc_data(void);
static uint8_t get_gps_gprmc_flag(void); 

static char* get_gps_time_str(void);
static int   get_gps_valid_val(void);
static char* get_gps_latitude(void);
static char* get_gps_longitude(void);
static float get_gps_speed_kn(void);

bsp_gps_handle_t gps =
{
	.handle = {	
		.rec_gps_data     = rec_gps_data_to_get_gprmc,
		.parse_gprmc_data = parse_gps_gprmc_data,
		.get_gprmc_flag   = get_gps_gprmc_flag,
	},
	.get_time_str   = get_gps_time_str,
	.get_valid_val = get_gps_valid_val,
	.get_latitude   = get_gps_latitude,
	.get_longitude  = get_gps_longitude,
	.get_speed_kn   = get_gps_speed_kn,
};

/* Private functions ---------------------------------------------------------*/

static void utc_time_to_local_time(const char utc_time[], const char utc_date[], \
												char *bjt_time, char *bjt_date)
{
	int hour, minute, second, day, month, year;
    int days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	
	// 解析UTC时间和日期字符串
    sscanf(utc_time, "%2d%2d%2d", &hour, &minute, &second);
    sscanf(utc_date, "%2d%2d%2d", &day, &month, &year);
	
	    // 闰年的2月是29天
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
        days_in_month[1] = 29;

    // 增加8小时
    hour += 8;

    // 处理超过24小时的情况
    if (hour >= 24) {
        hour -= 24;
        day += 1;
        // 处理月份的最后一天
        if (day > days_in_month[month - 1]) {
            day = 1;
            month += 1;
            // 处理年份的最后一天
            if (month > 12) {
                month = 1;
                year += 1;
            }
        }
    }

    // 格式化为北京时间字符串
    snprintf(bjt_date, 9, "%02d-%02d-%02d", year, month, day);
    snprintf(bjt_time, 9, "%02d:%02d:%02d", hour, minute, second);
}

static void rec_gps_data_to_get_gprmc(uint8_t data)
{
	static uint8_t index = 0;
	
	rec_gps_data[index++] = data;
	
	if (data == '\n')
	{
		rec_gps_data[index] = '\0';
		
		if (strncmp((const char *)rec_gps_data, "$GPRMC", 6) == 0)
		{
			g_gprmc_len = index;
			g_get_gprmc_flag = 1;
			
			memcpy(parse_gps_data, rec_gps_data, g_gprmc_len);
		    parse_gps_data[g_gprmc_len] = '\0';
		}
		
		index = 0;
	}
}

static void parse_gps_gprmc_data(void)
{
	char *start = (char*)parse_gps_data;
	char *end = NULL;
	
	char utc_time[11] = {0}, speed_val[8] = {0}, utc_date[7] = {0},
		bjt_time[9] = {0}, bjt_date[9] = {0};
	
	uint8_t gprmc_index = 0;
	uint8_t field_len;
	
	if (strncmp((const char *)parse_gps_data, "$GPRMC", 6) == 0)
	{
		while ((end = strstr((char *)start, ","))!= NULL)
		{
			field_len = (uint8_t)(end - start);
			
			switch(gprmc_index)
			{
				case UTC_TIME:
					field_len = CHECK_PARAM(utc_time, field_len);
					strncpy(utc_time, start, field_len);
					utc_time[sizeof(utc_time) - 1] = '\0';
				break;
				
				case VALID_VAL:
//					field_len = CHECK_PARAM(g_gps_param_t.valid_val, field_len);
//					strncpy(g_gps_param_t.valid_val, start, field_len);
//					g_gps_param_t.valid_val[sizeof(g_gps_param_t.valid_val) - 1] = '\0';
					if (strncmp("A", start, field_len) == 0)
					{
						g_gps_param_t.valid_val = 1;
					}
					else if (strncmp("V", start, field_len)== 0)
					{
						g_gps_param_t.valid_val = 0;
					}
					
				break;
				
				case LATITUDE_VAL:
					field_len = CHECK_PARAM(g_gps_param_t.latitude, field_len);
					strncpy(g_gps_param_t.latitude, start, field_len);
					g_gps_param_t.latitude[sizeof(g_gps_param_t.latitude) - 1] = '\0';
				break;
				
				case LATITUDE_TYPE:
					field_len = CHECK_PARAM(g_gps_param_t.latitude_type, field_len);
					strncpy(g_gps_param_t.latitude_type, start, field_len);
					g_gps_param_t.latitude_type[sizeof(g_gps_param_t.latitude_type) - 1] = '\0';
				break;
				
				case LONGITUDE_VAL:
					field_len = CHECK_PARAM(g_gps_param_t.longitude, field_len);
					strncpy(g_gps_param_t.longitude, start, field_len);
					g_gps_param_t.longitude[sizeof(g_gps_param_t.longitude) - 1] = '\0';
				break;
				
				case LONGITUDE_TYPE:
					field_len = CHECK_PARAM(g_gps_param_t.longitude_type, field_len);
					strncpy(g_gps_param_t.longitude_type, start, field_len);
					g_gps_param_t.longitude_type[sizeof(g_gps_param_t.longitude_type) - 1] = '\0';
				break;
				
				case SPEED_VAL:
					field_len = CHECK_PARAM(speed_val, field_len);
					strncpy(speed_val, start, field_len);
					speed_val[sizeof(speed_val) - 1] = '\0';
				break;
				
				case UTC_DATE:
					field_len = CHECK_PARAM(utc_date, field_len);
					strncpy(utc_date, start, field_len);
					utc_date[sizeof(utc_date) - 1] = '\0';
				break;
				
				default:
					
				break;
			}
			
			start = end + 1;
			gprmc_index++;
		}
		
		g_get_gprmc_flag = 0;
		
		// 解析时间
		utc_time_to_local_time(utc_time, utc_date, bjt_time, bjt_date);
		sprintf(g_gps_param_t.time_str, "%s %s", bjt_date, bjt_time);
		
		// 解析速度（节）
		sscanf(speed_val, "%f", &g_gps_param_t.speed_kn);
		

		
		// 记录全局变量
//		g_time_str = bjt_time;
//		g_date_str = bjt_date;
		
	
//		printf("UTC DATE = %s\r\n", utc_date);
//		printf("UTC TIME = %s\r\n", utc_time);
		
//		printf("GPS VALID IS: %s\r\n", valid_val);
//		
//		printf("Local Time = %s %s\r\n", g_date_str, g_time_str);
//		
//		printf("Longitude  = %s - %s\r\n", longitude_val, longitude_type);
//		printf("Latitude   = %s - %s\r\n", latitude_val, latitude_type);
//		
//		printf("Speed = %s\r\n", speed_val);
		
//		printf("------------------------------------------------\r\n");
		
	}	
}

static uint8_t get_gps_gprmc_flag(void)
{
	return g_get_gprmc_flag;
}

static char* get_gps_time_str(void)
{
	return g_gps_param_t.time_str;
}

static int get_gps_valid_val(void)
{
	return g_gps_param_t.valid_val;
}

static char* get_gps_latitude(void)
{
	return g_gps_param_t.latitude;
}
static char* get_gps_longitude(void)
{
	return g_gps_param_t.longitude;
}

static float get_gps_speed_kn(void)
{
	return g_gps_param_t.speed_kn;
}
