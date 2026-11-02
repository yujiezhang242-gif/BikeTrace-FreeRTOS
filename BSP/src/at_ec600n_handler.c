/******************************************************************************
 * Copyright (C) 2025 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file at_ec600n_driver.c
 * 
 * @par dependencies 
 * - at_ec600n_driver.h
 * 
 * @author Caprxw
 * 
 * @brief Functions related to get the hmi cmd and make the reaction.
 * 
 * Processing flow:
 * 
 * call directly.
 * 
 * @version V1.0 2025-04-19
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/
#include "ringbuffer.h"
#include "at_ec600n_handler.h"
#include "bsp_uart_driver.h"

#define AT_TIMEOUT 5000 // 超时设置，单位：毫秒

// 定义一个环形缓冲区
ringbuffer_t at_ringbuf;

/**
 * 发送透传数据
 * @param data 发送的数据
 * @param len  数据长度
 */
void at_passthrough_send(const char *data, int len)
{
	//HAL_UART_Transmit(&huart2, (uint8_t *)data, len, 5000);
	
	while(len--)
	{
		u2.sendByte(*data);
		data++;
	}
}

/**
 * 接收透传数据
 * @param buffer 缓冲区
 * @param size   缓冲区大小
 * @return 实际接收到的数据长度
 */
int at_passthrough_receive(char *buffer, int size)
{
    return ringbuffer_read(&at_ringbuf, buffer, size);
}

/**
 * 发送 AT 指令
 *
 * @param cmd 要发送的 AT 指令字符串（除了+++，均应以 '\r\n' 结尾）
 */
void at_send_cmd(char *cmd)
{
//	char *temp = cmd;
//    // 发送 AT 指令
//	while (*temp != '\0')
//	{
//		HAL_UART_Transmit(&huart2, (uint8_t *)temp, 1, 1000);
//		temp++;
//	}
	
	u2.sendString(cmd);
	
    //usart2_send_string(cmd);
    printf("at_send_cmd:%s\n", cmd);
}

/**
 * 接收字节数据
 *
 * @param byte 串口收到的字节数据
 */
void at_receive_byte(char byte)
{
    // 写入环形缓冲区
    ringbuffer_write(&at_ringbuf, byte);
}

/**
 * 读取响应数据
 *
 * @param succeeded_response 成功时的响应数据关键字或内容
 * @param failed_response 失败时的响应数据关键字或内容
 * @param timeout 超时时间（以 ms 为单位），传入-1表示一直等不超时
 * @param response_buffer 传出AT命令成功响应时的字符串
 * @param buffer_size response_buffer的大小
 * @return 响应结果：
 *         - 在响应数据中找到 succeeded_response 时返回 AT_OK
 *         - 找到 failed_response 时返回 AT_FAILED
 *         - 超时未收到响应时返回 AT_TIME_OUT
 */
AT_RESULT at_wait_for_response(const char *succeeded_response, const char *failed_response, int32_t timeout_ms
    , char *response_buffer, size_t buffer_size)
{
    uint32_t start_time = HAL_GetTick();  // 获取当前的系统毫秒数
    char buffer[512] = {0};              // 假设每次读取的最大数据长度为 512 字节
    volatile uint32_t buffer_offset = 0;               // 当前缓冲区中的数据长度
    int bytes_read = 0;                  // 一次读取到的数据

    // 持续读取直到超时
    while (1) {
        // 如果 timeout_ms 不为 -1，则检查是否超时
        if (timeout_ms != -1 && (HAL_GetTick() - start_time >= timeout_ms)) 
		{
            printf("at_wait_for_response Time out\n");
            return AT_TIME_OUT;
        }
		
        // 从环形缓冲区读取数据，直接写入 buffer 的剩余位置
        bytes_read = ringbuffer_read(&at_ringbuf, buffer + buffer_offset, sizeof(buffer) - buffer_offset - 1);
		
		//printf("bytes_read = [%d]\r\n", bytes_read);
		
		//if(buffer + buffer_offset != '\0')
			//printf("at_rec:%s", buffer + buffer_offset);
		
		if (bytes_read > 0) 
		{
            // 调试日志
			printf("at_rec:%s", buffer + buffer_offset);
            //printf("read [%d]\n", bytes_read);
            //print_hex_array(buffer + buffer_offset, bytes_read);
            // 更新偏移长度
            buffer_offset += bytes_read;

            // 检查是否包含成功响应
            if (succeeded_response && strstr(buffer, succeeded_response)) {
                if(response_buffer) {
                    // strncpy 不会在目标字符串的末尾添加空字符 '\0'
                    strncpy(response_buffer, buffer, buffer_size - 1);
                }
                return AT_OK;
            }

            // 检查是否包含失败响应
            if (failed_response && strstr(buffer, failed_response)) {
               // printf("at_wait_for_response AT_FAILED\r\n");
                return AT_FAILED;
            }

            // 如果未匹配到响应，则继续读取直到超时
        }
	}
}

/**
 * 执行 AT 指令并等待特定响应
 *
 * @param cmd 要执行的 AT 指令字符串（以 '\r\n' 结尾）
 * @param succeeded_response 成功响应的关键字或内容
 * @param failed_response 失败响应的关键字或内容
 * @param response_buffer 传出AT命令成功响应时的字符串
 * @param buffer_size response_buffer的大小
 * @return 执行结果，可能为 AT_OK、AT_FAILED、AT_ERROR 等
 */
AT_RESULT at_execute(char *cmd, const char *succeeded_response, const char *failed_response, 
    char *response_buffer, size_t buffer_size)
{
    if (response_buffer != NULL) {
        memset(response_buffer, 0, buffer_size); // 清空响应缓冲区
    }
    at_send_cmd(cmd); // 发送 AT 指令
    return at_wait_for_response(succeeded_response, failed_response, AT_TIMEOUT, response_buffer, buffer_size); // 等待并解析响应
}

/**
 * 初始化 AT 指令相关数据结构
 */
void at_init(void)
{
    /* 初始化环形缓冲区 */
    ringbuffer_init(&at_ringbuf);
}
