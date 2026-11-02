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
#include "at_ec600n_handler.h"
#include "at_ec600n_tcp_driver.h"
#include "delay.h"


typedef enum {
    ec600n_ECHO_OFF = 0,     // 对应 ATE0，关闭回显
    ec600n_ECHO_ON = 1       // 对应 ATE1，开启回显
} ec600n_ECHO_MODE;

const char *RESPONSE_OK = "OK\r\n";
const char *RESPONSE_ERROR = "ERROR";

/**
 * 发送数据
 *
 * @param data 要发送的数据
 * @param len 数据的长度
 */
void ec600n_send_data(const char *data, int len);
/**
 * 接收数据
 * @param buffer 缓冲区
 * @param size   缓冲区大小
 * @return 实际接收到的数据长度
 */
int ec600n_recv_data(char *buffer, int size);

/**
 * 退出透传模式，断开服务端，关闭PDP上下文
 * @return 执行结果
 */
static AT_RESULT ec600n_disconnect_server_shut(void);
/**
 * 返回 ec600n 是否连接到服务端
 */
static int get_ec600n_connected_server_status(void);
/**
 * 配置网络、激活PDP上下文、进入透传模式并连接服务器
 * @param ip        服务器 IP
 * @param port      服务器端口
 * @return 执行结果
 */
static AT_RESULT ec600n_activate_connect_server(const char *ip, int port);


bsp_ec600n_tcp_driver_t ec600n_tcp = 
{
	.init = at_init,
	.connectServer = ec600n_activate_connect_server,
	.disconnectServer = ec600n_disconnect_server_shut,
	.getconnectStatus = get_ec600n_connected_server_status,
	.send = ec600n_send_data,
	.recv = ec600n_recv_data,
};


int ec600n_is_connected_server = EC600N_NOT_CONNECTED_SERVER;

/**
 * 测试ec600n是否正常上电、准备就绪
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_ready(void)
{
    const char *cmd = "AT\r";
    return at_execute((char *)cmd, RESPONSE_OK, RESPONSE_ERROR, NULL, 0);
}
/**
 * 开启或关闭 AT 指令回显
 *
 * @param enable 1 开启，0 关闭
 * @return 执行结果
 */
static AT_RESULT ec600n_set_echo(ec600n_ECHO_MODE enable)
{
    const char *cmd = enable ? "ATE1\r" : "ATE0\r";
    return at_execute((char *)cmd, RESPONSE_OK, RESPONSE_ERROR, NULL, 0);
}
/**
 * 测试ec600n是否SIM卡解锁
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_sim_unlock(void)
{
    char status[10]; 
    char response[256];
    AT_RESULT result;
    
    result = at_execute("AT+CPIN?\r", RESPONSE_OK, RESPONSE_ERROR, response, sizeof(response));
    if (result != AT_OK) {
        printf("Failed to execute AT+CPIN\n");
        return result;
    }

    // 解析完整响应
    printf("AT+CPIN Response: %s\n", response);
    // 使用 sscanf 提取 STATUS 值
    char *pos = strstr(response, "+CPIN:");
    if (pos) {
        if (sscanf(pos, "+CPIN: %s", status) == 1) {
            if (strcmp(status, "READY") == 0) {
                return AT_OK;
            }
        } else {
            printf("AT+CPIN return wrong status:%s.\n", status);
        }
    }
    
    return AT_FAILED;
}
/**
 * 测试ec600n是否已注册到运营商网络
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_register(void)
{
    char status[10]; 
    char response[256];
    AT_RESULT result;
    
    result = at_execute("AT+CREG?\r", RESPONSE_OK, RESPONSE_ERROR, response, sizeof(response));
    if (result != AT_OK) {
        printf("Failed to execute AT+CREG\n");
        return result;
    }

    // 解析完整响应
    printf("AT+CREG Response: %s\n", response);
    // 使用 sscanf 提取 STATUS 值
    char *pos = strstr(response, "+CREG:");
    if (pos) {
        if (sscanf(pos, "+CREG: %s", status) == 1) {
            if (strcmp(status, "0,1") == 0) {
                return AT_OK;
            }
        } else {
            printf("AT+CREG return wrong status:%s.\n", status);
        }
    }
    
    return AT_FAILED;
}

/* ======================= 此处开始，剩余代码每个4G模块可能不同 ======================== */

/**
 * 配置TCP/IP场景参数 
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_attached(void)
{
    //char status[10]; 
    char response[256];
    AT_RESULT result;
    
    result = at_execute("AT+QICSGP=1,1\r", RESPONSE_OK, RESPONSE_ERROR, response, sizeof(response));
    if (result != AT_OK) {
        printf("Failed to execute AT+QICSGP\n");
        return result;
    }

    // 解析完整响应
    printf("AT+QICSGP Response: %s\n", response);
    // 使用 sscanf 提取 STATUS 值
    char *pos = strstr(response, "OK");
    if (pos) 
	{
		return AT_OK;
	}
	else 
	{
		printf("AT+QICSGP return fail\r\n");
	}  
    
    return AT_FAILED;
}
/**
 * 设置APN
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_set_apn(void)
{
    return at_execute("AT+CSTT\r", RESPONSE_OK, RESPONSE_ERROR, NULL, 0);
}
/**
 * 激活PDP上下文
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_activate_pdp(void)
{
    return at_execute("AT+QIACT=1\r", RESPONSE_OK, RESPONSE_ERROR, NULL, 0);
}
/**
 * 关闭PDP上下文
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_shut_pdp(void)
{
    return at_execute("AT+QIDEACT=1\r", RESPONSE_OK, RESPONSE_ERROR, NULL, 0);
}
///**
// * 设置透传模式
// * @return 执行结果
// */
//AT_RESULT ec600n_set_passthrough_mode(void)
//{
//    AT_RESULT result;
//    
//    result = at_execute("AT+CIPMODE=1\r", RESPONSE_OK, RESPONSE_ERROR, NULL, 0);
//    if (result != AT_OK) {
//        printf("Failed to execute AT+CIPMODE\n");
//        return result;
//    }
//    return AT_OK;
//}
/**
 * 退出透传模式
 *
 * 第一个 + 之前需要 1000ms 的间隔
 * 最后一个 + 之后需要 500ms 的间隔
 * 三个 + 之间的间隔不能超过 500ms
 */
static void ec600n_exit_passthrough_mode(void)
{
    Delay_Ms(1000);
    at_send_cmd("+++");  // 直接发送退出透传模式命令
    Delay_Ms(1000);
}
/**
 * 启动连接
 *
 * @param type 连接类型 ("TCP" 或 "UDP")
 * @param ip 目标服务器的 IP 地址
 * @param port 目标端口号
 * @return 执行结果
 */
static AT_RESULT ec600n_connect_server(const char *type, const char *ip, int port)
{
    char cmd[AT_COMMAND_MAX_LEN];
    snprintf(cmd, sizeof(cmd), "AT+QIOPEN=1,0,\"%s\",\"%s\",%d,0,2\r", type, ip, port);
    return at_execute(cmd, "CONNECT\r\n", RESPONSE_ERROR, NULL, 0);
}
/**
 * 断开与服务器的连接
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_disconnect_server()
{
    return at_execute("AT+QICLOSE\r", RESPONSE_OK, RESPONSE_ERROR, NULL, 0);
}

/**
 * 发送数据
 *
 * @param data 要发送的数据
 * @param len 数据的长度
 */
static void ec600n_send_data(const char *data, int len)
{
    at_passthrough_send(data, len);
}

/**
 * 接收数据
 * @param buffer 缓冲区
 * @param size   缓冲区大小
 * @return 实际接收到的数据长度
 */
static int ec600n_recv_data(char *buffer, int size)
{
    return at_passthrough_receive(buffer, size);
}

/**
 * 返回 ec600n 是否连接到服务端
 */
static int get_ec600n_connected_server_status(void)
{
    return ec600n_is_connected_server;
}

/**
 * 退出透传模式，断开服务端，关闭PDP上下文
 * @return 执行结果
 */
static AT_RESULT ec600n_disconnect_server_shut(void)
{
    AT_RESULT res;

//    if (ec600n_is_connected_server == EC600N_NOT_CONNECTED_SERVER)
//        return AT_OK;

    /* 不管是否成功，都将标记位设置为断开服务端，如果不这样设置，程序会更加复杂 */
    ec600n_is_connected_server = EC600N_NOT_CONNECTED_SERVER;
    
	// 1.发送+++退出透传模式
    ec600n_exit_passthrough_mode();
    
	// 2.发送QICLOSE退出TCP服务
    res = ec600n_disconnect_server();
    if (res != AT_OK) {
        return res;
    }
	
	// 3.发送QIDEACT退出PDP
    res = ec600n_shut_pdp();
    if (res != AT_OK) {
        return res;
    }   
    
    return AT_OK;
}
/**
 * 配置网络、激活PDP上下文、进入透传模式并连接服务器
 * @param ip        服务器 IP
 * @param port      服务器端口
 * @return 执行结果
 */
static AT_RESULT ec600n_activate_connect_server(const char *ip, int port)
{
    AT_RESULT res;
    static int n = 0;
    
    if (ec600n_is_connected_server == EC600N_CONNECTED_SERVER)
        return AT_OK;

    n++;

    if(n >= 10) { // 连续 10 次调用这个函数，都没有连上服务端，就重启模组
        n = 0;
        printf("Failed to connect to server 10 times, restarting module...\n");
        //ec600n_reset();
        return AT_TIME_OUT;
    }
    
	// 1.检查是否可以回应
    res = ec600n_ready();
    if (res != AT_OK) {
        printf("ec600n_ready failed: %d\n", res);
        return res;
    }
	
	// 2.检查是否可以回应(正式上线时关闭)
    res = ec600n_set_echo(ec600n_ECHO_OFF);
    if (res != AT_OK) {
        printf("ec600n_set_echo failed: %d\n", res);
        return res;
    }
	
	// 3.检查SIM卡是否解锁
    res = ec600n_sim_unlock();
    if (res != AT_OK) {
        printf("ec600n_sim_unlock failed: %d\n", res);
        return res;
    }
	
	// 4.检查SIM卡是否注册运营商
    res = ec600n_register();
    if (res != AT_OK) {
        printf("ec600n_register failed: %d\n", res);
        return res;
    }
	
	// 5.配置TCP/IP场景参数(这里有错的话可能是需要加上APN运营商)
    res = ec600n_attached();
    if (res != AT_OK) {
        printf("ec600n_attached failed: %d\n", res);
        return res;
    }
	
	// 激活前先去PDP上下文
	res = ec600n_shut_pdp();
	// 6.激活PDP上下文(可能返回错误，因为已经激活了)
	res = ec600n_activate_pdp();
//	if (res != AT_OK) {
//        printf("ec600n_activate_pdp failed: %d\n", res);
//        return res;
//    }
	
	// 7.连接TCP服务端并开启透传模式
    res = ec600n_connect_server("TCP", ip, port);
    if (res != AT_OK) {
        printf("ec600n_connect_server failed: %d\n", res);
        return res;
    }
    
    ec600n_is_connected_server = EC600N_CONNECTED_SERVER;
    n = 0;

    return AT_OK;
}

