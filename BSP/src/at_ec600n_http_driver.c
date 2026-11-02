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
#include "at_ec600n_http_driver.h"
#include "Delay.h"
#include "utils.h"

typedef enum {
    ec600n_ECHO_OFF = 0,     // 对应 ATE0，关闭回显
    ec600n_ECHO_ON = 1       // 对应 ATE1，开启回显
} ec600n_ECHO_MODE;

const char *HTTP_RESPONSE_OK = "OK\r\n";
const char *HTTP_RESPONSE_ERROR = "ERROR";

const char *bikeURL = "https://restapi.amap.com/v5/direction/bicycling?";
const char *walkURL = "https://restapi.amap.com/v5/direction/walking?";
const char *KEY = "a706c30f4ecae0b410041f5c5def7e31";

static int ec600n_is_connected_server = EC600N_NOT_CONNECTED_SERVER;

/**
 * 接收数据
 * @param buffer 缓冲区
 * @param size   缓冲区大小
 * @return 实际接收到的数据长度
 */
static int ec600n_recv_data(char *buffer, int size);

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
static AT_RESULT ec600n_activate_connect_server(void);

// static AT_RESULT ec600n_http_post(uint32_t strlen, uint32_t timeout);

static AT_RESULT ec600n_disconnect_post(void);

/**
* 重置PDP上下文
 *
 * @return 执行结果
 */
// static AT_RESULT ec600n_reset_pdp(void);

/**
* 发送路径规划GET请求
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_get_ride_path_request(char* gps_lat, char* gps_lon, double dest_lat, double dest_lon);

int abc;
bsp_ec600n_http_driver_t ec600n_http =
{
	.init = at_init,
	.connectServer = ec600n_activate_connect_server,
	.disconnectServer = ec600n_disconnect_server_shut,
	.getconnectStatus = get_ec600n_connected_server_status,
	.recv = ec600n_recv_data,
	.httpGetPath = ec600n_get_ride_path_request,
};

/**
 * 测试ec600n是否正常上电、准备就绪
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_ready(void)
{
    const char *cmd = "AT\r";
    return at_execute((char *)cmd, HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, NULL, 0);
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
    return at_execute((char *)cmd, HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, NULL, 0);
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
    
    result = at_execute("AT+CPIN?\r", HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, response, sizeof(response));
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
    
    result = at_execute("AT+CREG?\r", HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, response, sizeof(response));
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
/**
 * 配置TCP/IP场景参数 
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_attached(void)
{
    char response[256];
    AT_RESULT result;
    
    result = at_execute("AT+QICSGP=1,1\r", HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, response, sizeof(response));
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

///**
// * 设置APN
// *
// * @return 执行结果
// */
//static AT_RESULT ec600n_set_apn(void)
//{
//    return at_execute("AT+CSTT\r", HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, NULL, 0);
//}

/**
 * 激活PDP上下文
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_activate_pdp(void)
{
    return at_execute("AT+QIACT=1\r", HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, NULL, 0);
}
/**
 * 检查PDP上下文
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_check_pdp(void)
{
    char response[64];
    AT_RESULT result;
    
    result = at_execute("AT+QIACT?\r", HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, response, sizeof(response));
    if (result != AT_OK) {
        printf("Failed to execute AT+QIACT\n");
        return result;
    }

    // 完整响应
    printf("AT+QIACT? Response: %s\n", response);
	
	return AT_OK;
}
/**
 * 关闭PDP上下文
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_shut_pdp(void)
{
    return at_execute("AT+QIDEACT=1\r", HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, NULL, 0);
}

///**
//* 重置PDP上下文
// *
// * @return 执行结果
// */
//static AT_RESULT ec600n_reset_pdp(void)
//{
//	AT_RESULT tReturn = AT_OK;
//	
//    tReturn = ec600n_shut_pdp();
//	if (AT_OK != tReturn)
//	{
//		return AT_FAILED;
//	}
//	
//	tReturn = ec600n_activate_pdp();
//	if (AT_OK != tReturn)
//	{
//		return AT_FAILED;
//	}
//	
//	tReturn = ec600n_check_pdp();
//	if (AT_OK != tReturn)
//	{
//		return AT_FAILED;
//	}
//	
//	return AT_OK;	
//}

/**
 * 配置HTTP参数ID号
 *
 * 
 */
static AT_RESULT ec600n_http_config_contextid(void)
{
    char cmd[AT_COMMAND_MAX_LEN];
    snprintf(cmd, sizeof(cmd), "AT+QHTTPCFG=\"contextid\",1\r");
    return at_execute(cmd, HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, NULL, 0);
}
/**
* 配置HTTP请求头为禁用自定义
 *
 * 
 */
static AT_RESULT ec600n_http_config_requestheader(void)
{
    char cmd[AT_COMMAND_MAX_LEN];
    snprintf(cmd, sizeof(cmd), "AT+QHTTPCFG=\"requestheader\",0\r");
    return at_execute(cmd, HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, NULL, 0);
}
/**
* 配置HTTP传输为JSON数据格式
 *
 * 
 */
static AT_RESULT ec600n_http_config_contenttype(void)
{
    char cmd[AT_COMMAND_MAX_LEN];
    snprintf(cmd, sizeof(cmd), "AT+QHTTPCFG=\"contenttype\",4\r");
    return at_execute(cmd, HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, NULL, 0);
}
// 配置HTTP参数
static AT_RESULT ec600n_http_config(void)
{
	AT_RESULT tReturn = AT_OK;
	
	//Delay_Ms(1);
	
	tReturn = ec600n_http_config_contextid();
	if (AT_OK != tReturn)
	{
		printf("ec600n_http_config_contextid fail");
		return tReturn;
	}
	
	//Delay_Ms(1);
	
	tReturn = ec600n_http_config_requestheader();
	if (AT_OK != tReturn)
	{
		printf("ec600n_http_config_requestheader fail");
		return tReturn;
	}
	
	//Delay_Ms(1);
	
	tReturn = ec600n_http_config_contenttype();
	if (AT_OK != tReturn)
	{
		printf("ec600n_http_config_contenttype fail");
		return tReturn;
	}
	
	return AT_OK;
}

///**
//* 发送HTTP POST请求
// *
// * 
// */
//AT_RESULT ec600n_http_post(uint32_t strlen, uint32_t timeout)
//{
//    char cmd[AT_COMMAND_MAX_LEN];

//    snprintf(cmd, sizeof(cmd), "AT+QHTTPPOST=%d,%d,%d\r",strlen, 60, 60);
//    return at_execute(cmd, "CONNECT\r\n", HTTP_RESPONSE_ERROR, NULL, 0);
//}


// 1.https - ssl
static AT_RESULT ec600n_https_sslctxid(void)
{
    char cmd[AT_COMMAND_MAX_LEN];

    snprintf(cmd, sizeof(cmd), "AT+QHTTPCFG=\"sslctxid\",1\r");
    return at_execute(cmd, HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, NULL, 0);
}

// 2.https - ssl
static AT_RESULT ec600n_https_sslversion(void)
{
    char cmd[AT_COMMAND_MAX_LEN];

    snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"sslversion\",1,4\r");
    return at_execute(cmd, HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, NULL, 0);
}

// 3.https - ssl
static AT_RESULT ec600n_https_ciphersuite(void)
{
    char cmd[AT_COMMAND_MAX_LEN];

    snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"ciphersuite\",1,0XFFFF\r");
    return at_execute(cmd, HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, NULL, 0);
}

// 4.https - ssl
static AT_RESULT ec600n_https_seclevel(void)
{
    char cmd[AT_COMMAND_MAX_LEN];

    snprintf(cmd, sizeof(cmd), "AT+QSSLCFG=\"seclevel\",1,0\r");
    return at_execute(cmd, HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, NULL, 0);
}

/**
* 发送HTTP GET请求
 *
 * 
 */
AT_RESULT ec600n_http_get(uint32_t timeout)
{
    char cmd[AT_COMMAND_MAX_LEN];

    snprintf(cmd, sizeof(cmd), "AT+QHTTPGET=%d\r",timeout);
    return at_execute(cmd, HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, NULL, 0);
}

/**
* 发送HTTP 读取数据GET请求
 *
 * 
 */
AT_RESULT ec600n_http_get_read(uint32_t timeout)
{
    char cmd[AT_COMMAND_MAX_LEN];

    snprintf(cmd, sizeof(cmd), "AT+QHTTPREAD=%d\r",timeout);
    return at_execute(cmd, "CONNECT\r\n", HTTP_RESPONSE_ERROR, NULL, 0);
}

/**
* 配置HTTP URL
 *
 * 
 */
static AT_RESULT ec600n_http_config_url(uint32_t urllen, uint32_t timeout)
{
    char cmd[AT_COMMAND_MAX_LEN];

    snprintf(cmd, sizeof(cmd), "AT+QHTTPURL=%d,%d\r",urllen, timeout);
    return at_execute(cmd, "CONNECT", HTTP_RESPONSE_ERROR, NULL, 0);
}

static AT_RESULT ec600n_http_set_url(char *url)
{
	AT_RESULT tReturn = AT_OK;
	uint32_t urllen = strlen(url);
	
	printf("\r\nURL = %s, URLlen = [%d]\r\n", url, urllen);
	
	tReturn = ec600n_http_config_url(urllen, 5);
	if (AT_OK != tReturn)
	{
		printf("ec600n_http_config_url fail");
		return tReturn;
	}
    at_passthrough_send(url,urllen);
	printf("send url succ!\r\n");
	
	return AT_OK;
}

/**
* 发送路径规划GET请求
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_get_ride_path_request(char* gps_lat, char* gps_lon, double dest_lat, double dest_lon)
{
	double gcj_lat, gcj_lon;
	char gcj_str_url[256];
	
	get_gcj(gps_lat, gps_lon, &gcj_lat, &gcj_lon);
	
	sprintf(gcj_str_url, "%sorigin=%.6f,%.6f&destination=%.6f,%.6f&key=%s", \
		bikeURL, gcj_lat, gcj_lon, dest_lat, dest_lon, KEY);
	
	///////////////////////////////////////////////////////////////////////////////////
	//char *testurl = "https://httpbin.org/get";
	
	if (AT_OK != ec600n_http_set_url(gcj_str_url))
	{
		printf("ec600n_http_set_url fail!\r\n");
		return AT_FAILED;
	}
	
	//HAL_Delay(1000);
	
	if (AT_OK != ec600n_http_get(5))
	{
		printf("ec600n_http_get fail!\r\n");
		return AT_FAILED;
	}
	
	// 需要延迟一段时间
	HAL_Delay(3000);
	
	if (AT_OK != ec600n_http_get_read(30))
	{
		//printf("ec600n_http_get_read fail!\r\n");
		return AT_FAILED;
	}
	
	HAL_Delay(300);
	
	return AT_OK;
}


/**
 * 断开与服务器的连接
 *
 * @return 执行结果
 */
static AT_RESULT ec600n_disconnect_post()
{
    return at_execute("AT+QHTTPSTOP\r", HTTP_RESPONSE_OK, HTTP_RESPONSE_ERROR, NULL, 0);
}

///**
// * 发送数据
// *
// * @param data 要发送的数据
// * @param len 数据的长度
// */
//static void ec600n_send_data(const char *data, int len)
//{
//    at_passthrough_send(data, len);
//}

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
		
	// 发送给串口屏
	//printf("net_state_n.val=0\xff\xff\xff");
    
	// 1.发送+++退出透传模式
    //ec600n_exit_passthrough_mode();
    
	// 2.发送QICLOSE退出TCP服务
    res = ec600n_disconnect_post();
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
static AT_RESULT ec600n_activate_connect_server(void)
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
	
	res = ec600n_check_pdp();
	if (res != AT_OK)
	{
		// 6.激活PDP上下文(可能返回错误，因为已经激活了)
		res = ec600n_activate_pdp();
	}

//	if (res != AT_OK) {
//        printf("ec600n_activate_pdp failed: %d\n", res);
//        return res;
//    }
	
//	// 7.连接TCP服务端并开启透传模式
//    res = ec600n_connect_server("TCP", ip, port);
//    if (res != AT_OK) {
//        printf("ec600n_connect_server failed: %d\n", res);
//        return res;
//    }、
	
	// 7.配置HTTP URL
	//ec600n_http_set_url();
	
	// 8.配置HTTP
	res = ec600n_http_config();
	if (res != AT_OK) {
        printf("ec600n_http_config failed: %d\n", res);
//		// 3.发送QIDEACT退出PDP
//		res = ec600n_shut_pdp();
//		HAL_Delay(2);
//		ec600n_activate_pdp();
        return res;
    }
	// 将网络状态设置为连接
    ec600n_is_connected_server = EC600N_CONNECTED_SERVER;
	
	// 发送给串口屏
	//printf("net_state_n.val=1\xff\xff\xff");
	
	// 1.https - ssl
	res = ec600n_https_sslctxid();
	if (res != AT_OK) {
        printf("ec600n_https_sslctxid failed: %d\n", res);
        return res;
    }

// 2.https - ssl
	res = ec600n_https_sslversion();
	if (res != AT_OK) {
        printf("ec600n_https_sslversion failed: %d\n", res);
        return res;
    }

// 3.https - ssl
	res = ec600n_https_ciphersuite();
	if (res != AT_OK) {
        printf("ec600n_https_ciphersuite failed: %d\n", res);
        return res;
    }

// 4.https - ssl
	res = ec600n_https_seclevel();
	if (res != AT_OK) {
        printf("ec600n_https_seclevel failed: %d\n", res);
        return res;
    }
	
    n = 0;

    return AT_OK;
}

