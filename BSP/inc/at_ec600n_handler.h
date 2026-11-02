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
#ifndef __AT_INTERFACE_H
#define __AT_INTERFACE_H

#include <stdio.h>
#include <string.h>
#include "usart.h"

typedef enum {
    AT_OK                   = 0,    // 执行成功
    AT_FAILED               = 1,    // 执行失败
    AT_UNKNOWN_RESPONSE     = 2,    // 返回了未知的响应数据
    AT_TIME_OUT             = 3,    // 执行超时
    AT_BAD_PARAMETER        = 4     // 参数有误
} AT_RESULT;

/**
 * 发送POST请求
 * @param data 发送的数据
 * @param len  数据长度
 */
AT_RESULT send_Post(const char *data, int len);
	
/**
 * 发送透传数据
 * @param data 发送的数据
 * @param len  数据长度
 */
void at_passthrough_send(const char *data, int len);
/**
 * 接收透传数据
 * @param buffer 缓冲区
 * @param size   缓冲区大小
 * @return 实际接收到的数据长度
 */
int at_passthrough_receive(char *buffer, int size);
/**
 * 发送 AT 指令
 *
 * @param cmd 要发送的 AT 指令字符串（除了+++，均应以 '\r\n' 结尾）
 */
void at_send_cmd(char *cmd);
/**
 * 接收字节数据
 *
 * @param byte 串口收到的字节数据
 */
void at_receive_byte(char byte);
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
    , char *response_buffer, size_t buffer_size);
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
    char *response_buffer, size_t buffer_size);
/**
 * 初始化 AT 指令相关数据结构
 */
void at_init(void);
#endif
