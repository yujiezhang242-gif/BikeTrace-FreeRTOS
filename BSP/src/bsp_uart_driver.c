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
#include "bsp_uart_driver.h"
#include "bsp_gps_driver.h"
#include "bike.h"

#include "w25qxx.h"
#include "bsp_at24c02_driver.h"
#include "at_ec600n_handler.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static uint8_t debug_rx_data;
static uint8_t u2_rx_data;
static uint8_t u3_rx_data;

/* Private function prototypes -----------------------------------------------*/

static void u2_start(void);
static void u2_send_byte(uint8_t data);
static void u2_send_array(uint8_t *data, uint16_t len);
static void u2_send_string(char *str);
static uint8_t u2_get_receive_data(void);

static void u3_start(void);
static void u3_send_byte(uint8_t data);
static void u3_send_array(uint8_t *data, uint16_t len);
static void u3_send_string(char *str);
static uint8_t u3_get_receive_data(void);

bsp_uart_driver_t u2 = 
{
	.start      =  u2_start,
	.sendByte   =  u2_send_byte,
	.sendArr    =  u2_send_array,
	.sendString =  u2_send_string,
	.getRxData  =  u2_get_receive_data,
};

bsp_uart_driver_t u3 = 
{
	.start      =  u3_start,
	.sendByte   =  u3_send_byte,
	.sendArr    =  u3_send_array,
	.sendString =  u3_send_string,
	.getRxData  =  u3_get_receive_data,
};

/* Private functions ---------------------------------------------------------*/

void debug_uart_init()
{
	HAL_UART_Receive_IT(&huart1, &debug_rx_data, 1);
}

/* *********************** 调试用 ************************ */

static void read_at24_data(void)
{
	uint8_t page_index, write_flag;
	uint16_t record_cnt;
	int16_t kx,ky;
	double dest_lat, dest_lng;
	
	at24c02.readBytes(&page_index, 0, 1);
	at24c02.readBytes(&write_flag, 1, 1);
	at24c02.readBytes((uint8_t *)&record_cnt, 2, 2);
	at24c02.readBytes((uint8_t *)&kx,4,2);
	at24c02.readBytes((uint8_t *)&ky,6,2);
	at24c02.readBytes((uint8_t *)&dest_lat,8,2);
	at24c02.readBytes((uint8_t *)&dest_lng,16,2);
	
	printf("========== at24 data ==========\r\n");
	
	printf("page_index = [%d]\r\n", page_index);
	printf("write_flag = [%x]\r\n", write_flag);	
	printf("record_cnt = [%d]\r\n", record_cnt);
	printf("kx = [%d]\r\n", kx);
	printf("ky = [%d]\r\n", ky);	
	printf("dest_lat = [%f]\r\n", dest_lat);
	printf("dest_lng = [%f]\r\n", dest_lng);
	
	printf("========== at24 data ==========\r\n");
}

void cmd_revevied(uint8_t rx_data)
{
    #define RX_BUFFER_SIZE 16
    static char rx_buffer[RX_BUFFER_SIZE];
    static uint16_t rx_index = 0;
    
    rx_buffer[rx_index++] = rx_data;

    // 检测到换行符，表示一条命令
    if (rx_data == '\n')
    {
        rx_buffer[rx_index] = '\0'; // 确保字符串以NULL结尾

        //检查是否是读取数据的命令
        if (strncmp(rx_buffer, "get data", 8) == 0)
        {
            bike_data_list_request();
        }
        else if (strncmp(rx_buffer, "clear data", 10) == 0)
		{
			BSP_W25Qx_Erase_Chip();
			at24c02.erase();
			printf("at24 and w25q clear done!\r\n");
		}
		else if (strncmp(rx_buffer, "read at24", 9) == 0)
		{
			read_at24_data();
		}
		else if (strncmp(rx_buffer, "check status", 12) == 0)
		{
			print_restore_status_info();
		}
		else if (strncmp(rx_buffer, "normal mode", 11) == 0)
		{
			write_write_flag_to_at24();
		}
		else
        {
            bike_data_exprot_request(rx_buffer);
        }

        rx_index = 0; // 复位索引，准备接收下一条句子
    }
}


/* *********************** USART2 ************************ */
static void u2_start(void)
{
	HAL_UART_Receive_IT(&huart2, &u2_rx_data, 1);
}

static void u2_send_byte(uint8_t data)
{
	HAL_UART_Transmit(&huart2, &data, 1, 100);
}

static void u2_send_array(uint8_t *data, uint16_t len)
{
	if(data == NULL)
	{
		return;
	}
	
	while(len--)
	{
		u2_send_byte(*data);
		data++;
	}
}

static void u2_send_string(char *str)
{
	while(*str)
		u2_send_byte(*(uint8_t *)str++);
}

static uint8_t u2_get_receive_data(void)
{
	return u2_rx_data;
}


/* *********************** USART3 ************************ */
static void u3_start(void)
{
	HAL_UART_Receive_IT(&huart3, &u3_rx_data, 1);
}

static void u3_send_byte(uint8_t data)
{
	HAL_UART_Transmit(&huart3, &data, 1, 100);
}

static void u3_send_array(uint8_t *data, uint16_t len)
{
	if(data == NULL)
	{
		return;
	}
	
	while(len--)
	{
		u3_send_byte(*data);
		data++;
	}
}

static void u3_send_string(char *str)
{
	while(*str)
		u3_send_byte(*(uint8_t *)str++);
}

static uint8_t u3_get_receive_data(void)
{
	return u3_rx_data;
}


/* 接收完成回调函数 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    // 将接收到的数据回传
	  
    cmd_revevied(debug_rx_data);
    
    // 重新启用中断接收
    HAL_UART_Receive_IT(&huart1, &debug_rx_data, 1);
  }
  else if (huart->Instance == USART2) {
    // 将接收到的数据回传
	uint8_t rx_data = u2_get_receive_data();
	  
	at_receive_byte(rx_data);
	  
	  //LED_GPS_TOGGLE();
    
    // 重新启用中断接收
    u2.start();
  }
  else if (huart->Instance == USART3) {
    // 将接收到的数据回传
	uint8_t rx_data = u3_get_receive_data();
	  
    gps.handle.rec_gps_data(rx_data);
    
    // 重新启用中断接收
    u3.start();
  }
}
