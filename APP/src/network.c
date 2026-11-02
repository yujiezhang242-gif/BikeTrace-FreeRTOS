/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file network.c
 * 
 * @par dependencies 
 * - network.h
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
#include "network.h"
#include "cJSON.h"
#include "utils.h"
#include "bike.h"

#include "bsp_at24c02_driver.h"
#include "bsp_gps_driver.h"

#include "at_ec600n_tcp_driver.h"

// 定义结构体 - 12字节
typedef struct {
    char instruction[4]; // 存储英文指示词，如 "L", "R"
    char orientation[4]; // 存储英文方向，如 "NE", "E"
    uint32_t step_distance;
} Step;

typedef struct {
    Step* steps;          // 动态数组存储 steps
    uint32_t total_distance; // 总距离
    uint32_t total_seconds; // 总时长
	double dest_lat;      // 目的地纬度
	double dest_lng;      // 目的地经度
    int step_count;       // 步骤数量
} Path;

static Path path = {0};

static uint8_t g_generate_ride_path_done_flag = 0;
static uint8_t g_cloud_send = 0;
static uint8_t g_cloud_response = 0;

/* 以下是新增代码 */
/******************************************************************************************************/
/******************************************************************************************************/
/******************************************************************************************************/

const char bike_name[16] = "dev_bike";     		   // 设备名称
const char bike_type[8]  = "bike";                 // 设备类型

// 判断字符串是否为合法 JSON，并解析 "cmd" 字段
// 解析成功返回1，否则返回0
int parse_json_cmd(const char *json_str, char *cmd_out, size_t cmd_out_size)
{
    // 检查输入参数的合法性
    if (!json_str || !cmd_out || cmd_out_size == 0) {
        return 0; // 参数无效
    }

    // 尝试解析 JSON
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        return 0; // 解析失败
    }

    // 尝试获取 "cmd" 字段
    cJSON *cmd = cJSON_GetObjectItem(json, "cmd");
    if (!cmd || !cJSON_IsString(cmd)) {
        cJSON_Delete(json);
        return 0; // 没有 "cmd" 字段，或字段类型不是字符串
    }

    // 将 "cmd" 的值复制到输出缓冲区（确保不会越界）
    strncpy(cmd_out, cmd->valuestring, cmd_out_size - 1);
    cmd_out[cmd_out_size - 1] = '\0'; // 确保字符串以 '\0' 结尾

    // 释放 JSON 对象
    cJSON_Delete(json);

    // 返回成功标志
    return 1;
}

/* ================== 解析时间戳 =================== */
int parse_json_time(const char *json_str, uint32_t* time_out)
{
	// 检查输入参数的合法性
    if (!json_str || !time_out) {
        return 0; // 参数无效
    }

    // 尝试解析 JSON
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        return 0; // 解析失败
    }

    // 尝试获取 "time" 字段
    cJSON *time = cJSON_GetObjectItem(json, "time");
    if (!time || !cJSON_IsNumber(time)) {
        cJSON_Delete(json);
        return 0; // 没有 "time" 字段，或字段类型不是数字
    }

    // 提取 "time" 字段的值
    *time_out = (uint32_t)time->valuedouble;

    // 释放 JSON 对象
    cJSON_Delete(json);

    // 返回成功标志
    return 1;
}

/* ================== 解析路径规划信息，存入结构体 =================== */
int parse_json_path_plan(const char *json_str, Path *path) {
    // 检查输入参数的合法性
    if (!json_str || !path) {
        return 0; // 参数无效
    }

    // 尝试解析 JSON
    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        return 0; // 解析失败
    }
	
	printf("rec_path_plan is JSON!\r\n");
	
    // 获取 "data" 对象
    cJSON *data = cJSON_GetObjectItem(json, "data");
    if (!data || !cJSON_IsObject(data)) {
        cJSON_Delete(json);
        return 0;
    }
	
	printf("get JSON-data!\r\n");
	
    // 获取 "paths" 对象
    cJSON *paths = cJSON_GetObjectItem(data, "paths");
    if (!paths || !cJSON_IsObject(paths)) {
        cJSON_Delete(json);
        return 0;
    }
	
	printf("get JSON-paths!\r\n");

    // 获取 "distance"
    cJSON *distance = cJSON_GetObjectItem(paths, "distance");
    if (!distance || !cJSON_IsNumber(distance)) {
        cJSON_Delete(json);
        return 0;
    }
    path->total_distance = (uint32_t)distance->valuedouble;
	
	printf("get JSON-distance!\r\n");

    // 获取 "duration"
    cJSON *duration = cJSON_GetObjectItem(paths, "duration");
    if (!duration || !cJSON_IsNumber(duration)) {
        cJSON_Delete(json);
        return 0;
    }
    path->total_seconds = (uint32_t)duration->valuedouble;
	
	printf("get JSON-duration!\r\n");
	
	// Get "dest_lat"
    cJSON *dest_lat = cJSON_GetObjectItem(paths, "dest_lat");
    if (!dest_lat || !cJSON_IsNumber(dest_lat)) {
        cJSON_Delete(json);
        return 0;
    }
    path->dest_lat = dest_lat->valuedouble;
	
	printf("get JSON-dest_lat!\r\n");

    // Get "dest_lng"
    cJSON *dest_lng = cJSON_GetObjectItem(paths, "dest_lng");
    if (!dest_lng || !cJSON_IsNumber(dest_lng)) {
        cJSON_Delete(json);
        return 0;
    }
    path->dest_lng = dest_lng->valuedouble;
	
	printf("get JSON-dest_lng!\r\n");

    // 获取 "steps" 数组
    cJSON *steps_array = cJSON_GetObjectItem(paths, "steps");
    if (!steps_array || !cJSON_IsArray(steps_array)) {
        cJSON_Delete(json);
        return 0;
    }
	
	printf("get JSON-steps!\r\n");

    // 获取步骤数量
    path->step_count = cJSON_GetArraySize(steps_array);
    if (path->step_count <= 0) {
        cJSON_Delete(json);
        return 0;
    }
	
	printf("get step_count!\r\n");

    // 分配内存给 steps - 移植RTOS后需要替换为pvmalloc
    path->steps = (Step*)malloc(sizeof(Step) * path->step_count);
    if (!path->steps) {
        cJSON_Delete(json);
        return 0;
    }
	
	printf("get steps memory succ!\r\n");

    // 遍历数组并解析每个步骤
    for (int i = 0; i < path->step_count; i++) {
        cJSON *step_item = cJSON_GetArrayItem(steps_array, i);
        if (!step_item || !cJSON_IsObject(step_item)) {
            free(path->steps);
            cJSON_Delete(json);
            return 0;
        }

        // 获取 "inst"
        cJSON *inst = cJSON_GetObjectItem(step_item, "inst");
        if (!inst || !cJSON_IsString(inst)) {
            free(path->steps);
            cJSON_Delete(json);
            return 0;
        }
        strncpy(path->steps[i].instruction, inst->valuestring, 3);
        path->steps[i].instruction[3] = '\0';

        // 获取 "orie"
        cJSON *orie = cJSON_GetObjectItem(step_item, "orie");
        if (!orie || !cJSON_IsString(orie)) {
            free(path->steps);
            cJSON_Delete(json);
            return 0;
        }
        strncpy(path->steps[i].orientation, orie->valuestring, 3);
        path->steps[i].orientation[3] = '\0';

        // 获取 "dist"
        cJSON *dist = cJSON_GetObjectItem(step_item, "dist");
        if (!dist || !cJSON_IsNumber(dist)) {
            free(path->steps);
            cJSON_Delete(json);
            return 0;
        }
        path->steps[i].step_distance = (uint32_t)dist->valuedouble;
		
		printf("finish [%d] steps parase!\r\n", i);
    }

    // 释放 JSON 对象
    cJSON_Delete(json);

    // 返回成功标志
    return 1;
}


int bike_handle_server_data(const char *json_str)
{
	/* ================== cmd需要增大缓冲区大小 =================== */
    char cmd[1024] = {0}; // 用于存储解析到的 cmd 值

    if (parse_json_cmd(json_str, cmd, sizeof(cmd))) {
        printf("receive cmd [%s] from json:%s\n", cmd, json_str);
        if (strcmp(cmd, "time response") == 0) {
            uint32_t timestamp_cloud;
            if(parse_json_time(json_str, &timestamp_cloud)) {
                // 更新本地时间戳
                bike_set_local_time_val(timestamp_cloud + 8 * 3600);
				printf("update timestamp_local succ: %u", bike_get_local_time_val());
            }
        } 
		/* ================== 增加path_plan =================== */
		else if (strcmp(cmd, "path_plan") == 0) {
			if (parse_json_path_plan(json_str, &path)) {
		
				  // 置解析完成标志位
				  g_generate_ride_path_done_flag = 1;
				
				  // 记录目的地经纬度
				  at24c02.writeBytes((uint8_t *)&path.dest_lat, DEST_LAT_ADDR, 8);
				  printf("path.dest_lat = [%f]\r\n", path.dest_lat);
				  HAL_Delay(200);
				  at24c02.writeBytes((uint8_t *)&path.dest_lng, DEST_LNG_ADDR, 8);
				  printf("path.dest_lng = [%f]\r\n", path.dest_lng);
				  // 打印调试信息
//                printf("Path parsed: total_distance=%u, total_seconds=%u, step_count=%d\n",
//                       path.total_distance, path.total_seconds, path.step_count);
//                for (int i = 0; i < path.step_count; i++) {
//                    printf("Step %d: inst=%s, orie=%s, dist=%u\n",
//                           i, path.steps[i].instruction, path.steps[i].orientation, path.steps[i].step_distance);
//                }
                // 释放动态分配的内存
                // free(path.steps);
            } else {
                printf("Failed to parse path_plan\n");
            }
			
		} else {
            printf("cmd not support\n");
        }
        return 0;
    } else {
        // 没有收到一条完整的json
        return -1;
    }
}

void bike_cloud_recv_data(void)
{
    #define RECV_BUF_SIZE 2048               // 定义接收缓冲区大小
    static char recv_buf[RECV_BUF_SIZE];    // 接收缓冲区
    static int total_len = 0;               // 缓冲区收到数据总长度
    int recv_len;                           // 单次接收数据长度
	
	static uint32_t last_recv_tick = 0;
    
	// 调试时开启注释
//    if (!ec600n_tcp.getconnectStatus()) // 没有连到服务器，不收数据
//        return;

    recv_len = ec600n_tcp.recv(recv_buf + total_len, RECV_BUF_SIZE - total_len - 1);
	
	// 接收时间限制
	if ((HAL_GetTick() - last_recv_tick) > 1500)
	{
		total_len = 0;   // 清空缓冲区
	}
	
    if(recv_len) {
		printf("recv_len = [%d]\r\n", recv_len);
        total_len += recv_len;
        recv_buf[total_len] = '\0';       // 确保以 null 结尾
        printf("receive [%d] data:%s\r\n", total_len, recv_buf);
	
		last_recv_tick = HAL_GetTick();
		
        if (bike_handle_server_data(recv_buf) == 0) {
            total_len = 0;            // 清空缓冲区
        } else {
            // 收到的数据不是一个完整 json 字符串
            printf("Invalid JSON, waiting for more data...\r\n");
        }
		if (total_len >= RECV_BUF_SIZE)
		{
			total_len = 0;
		}
    }
}

/* ================== 数据上传 =================== */
/**
 * @brief 添加带浮点精度限制的数字到 cJSON 对象中
 * 
 * @param obj 目标 JSON 对象
 * @param key 键名
 * @param value 浮点值
 * @param precision 保留小数点位数
 */
void cJSON_AddNumberWithPrecision(cJSON *obj, const char *key, double value, int precision) {
    char buffer[32]; // 足够存储格式化的浮点数
    snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
    cJSON_AddItemToObject(obj, key, cJSON_CreateRaw(buffer));
}
/**
 * @brief 将 bike 数据结构组装成 JSON 字符串。
 *  {"cmd":"upload data","name":"dev_bike_kieran","type":"bike","time":1735376027,
    "data":{"temp":24.05,"humidity":22.64,"speed":33.5,"distance":10.44,
    "total_time":120,"state":0,"max_speed":30,"average_speed":30,"kcal":30,
    "lat":39.999861,"lng":116.484648}} 
 * @return char* 生成的 JSON 字符串，需手动 free
 */
char* encode_bike_upload(void)
{
    double gcj_lat, gcj_lon;
    // 创建根 JSON 对象
    cJSON *json_root = cJSON_CreateObject();
    if (json_root == NULL) {
        return NULL;
    }

    // 添加基础字段
    cJSON_AddStringToObject(json_root, "cmd", "upload data");
    cJSON_AddStringToObject(json_root, "name", bike_name);
    cJSON_AddStringToObject(json_root, "type", bike_type);
    cJSON_AddNumberToObject(json_root, "time", bike_get_local_time_val() - 8 * 3600);

    // 创建 data 子对象
    cJSON *json_data = cJSON_CreateObject();
    if (json_data == NULL) {
        cJSON_Delete(json_root);
        return NULL;
    }

    // 添加数据到 data 子对象，控制浮点精度为 2 位
    cJSON_AddNumberWithPrecision(json_data, "temp", bike_get_temp(), 2);
    cJSON_AddNumberWithPrecision(json_data, "humidity", bike_get_humi(), 2);
    cJSON_AddNumberWithPrecision(json_data, "speed", bike_get_speed_kmh(), 2);
    cJSON_AddNumberWithPrecision(json_data, "distance", bike_get_distance_km(), 2);
    cJSON_AddNumberToObject(json_data, "total_time", bike_get_total_time_sec());
    cJSON_AddNumberWithPrecision(json_data, "max_speed", bike_get_max_speed_kmh(), 2);
    cJSON_AddNumberWithPrecision(json_data, "average_speed", bike_get_average_speed_kmh(), 2);
    cJSON_AddNumberWithPrecision(json_data, "kcal", bike_get_kcal(), 2);

	/* ************** 转换为火星坐标系，且控制浮点精度为 6 位 *************** */
    get_gcj(gps.get_latitude(), gps.get_longitude(), &gcj_lat, &gcj_lon);
    cJSON_AddNumberWithPrecision(json_data, "lat", gcj_lat, 6);
    cJSON_AddNumberWithPrecision(json_data, "lng", gcj_lon, 6);

    // 将 data 子对象添加到根对象
    cJSON_AddItemToObject(json_root, "data", json_data);

    // 转换成 JSON 字符串
    char *json_string = cJSON_PrintUnformatted(json_root);

    // 释放 JSON 对象
    cJSON_Delete(json_root);

    return json_string;  // 返回动态分配的 JSON 字符串
}

void bike_cloud_upload_data(void)
{
    // 组装 JSON
    char *json_str = encode_bike_upload();
    if (json_str != NULL) {
        printf("*************** upload data ***************\n");
        printf("%s\n", json_str);
        ec600n_tcp.send(json_str, strlen(json_str));
        free(json_str);  // 释放内存
    } else {
        printf("Failed to generate JSON.\n");
    }
}

/* ================== 请求时间戳同步 =================== */
void bike_cloud_time_request(void)
{
    // 组装 JSON，比较简单，不使用 cJSON库函数
    char json_str[64];
	sprintf(json_str, "{\"cmd\":\"time request\",\"name\":\"%s\"}", bike_name);
    printf("*************** request time ***************\n");   
    ec600n_tcp.send(json_str, strlen(json_str));
}

/* ================== 请求路径规划 =================== */
void bike_path_plan_request(void)
{
	char json_str[64];
	double dest_lat, dest_lng;
//	at24c02.readBytes((uint8_t *)&dest_lat, DEST_LAT_ADDR, 8);
//	at24c02.readBytes((uint8_t *)&dest_lng, DEST_LNG_ADDR, 8);
	dest_lat = 22.90908;
	dest_lng = 113.878915;
	sprintf(json_str, "{\"cmd\":\"request_path_plan\",\"lat\":%.6f,\"lng\":%.6f}", dest_lat, dest_lng);
	
    printf("*************** request path plan ***************\n");   
    ec600n_tcp.send(json_str, strlen(json_str));
}

/* ================== 外部 =================== */

/**
 * 查看路径是否已经生成完毕
 * @param
 * @return 返回标志
 */
uint8_t check_ride_path_gener_done(void)
{
	return g_generate_ride_path_done_flag;
}

/**
 * 清除路径规划完成标志
 * @param
 * @return 返回标志
 */
uint8_t clear_ride_path_done_flag(void)
{
	g_generate_ride_path_done_flag = 0;
}

/**
 * 获取总距离和总时长数据
 * @param
 * @return 
 */
void get_total_ride_info(uint32_t *total_distance, uint32_t *total_seconds)
{
	*total_distance = path.total_distance;
	*total_seconds = path.total_seconds;
}

/**
 * 获取第n个步骤的相关信息
 * @param
 * @return 
 */
void get_step_n_info(uint16_t step_n, char **inst, char **orie, uint32_t *step_distance)
{
	if (path.steps == NULL) return;
	
	*inst = path.steps[step_n].instruction;
	*orie = path.steps[step_n].orientation;
	*step_distance = path.steps[step_n].step_distance;
}

/**
 * 获取共有步骤数量
 * @param
 * @return 
 */
int get_step_num(void)
{
	return path.step_count;
}

/**
 * 释放路径规划内存
 * @param
 * @return 
 */
void reset_path_memory(void)
{
	if (path.steps != NULL)
		free(path.steps);
	
	path.step_count     = 0;
	path.total_distance = 0;
	path.total_seconds  = 0;
}

/**
*  解析指示词返回枚举
 * @param
 * @return 
 */
INST_CASE parse_instruction(const char* input) {
	
	if (input == NULL) return INSTRUCTION_NONE;

    if (strcmp(input, "R")  == 0) return INSTRUCTION_RIGHT;
    if (strcmp(input, "L")  == 0) return INSTRUCTION_LEFT;
    if (strcmp(input, "UT") == 0) return INSTRUCTION_UTURN;
    if (strcmp(input, "LF") == 0) return INSTRUCTION_LEFT_FRONT;
    if (strcmp(input, "RF") == 0) return INSTRUCTION_RIGHT_FRONT;
    if (strcmp(input, "DT") == 0) return INSTRUCTION_DESTINATION;
    if (strcmp(input, "ST") == 0) return INSTRUCTION_STRAIGHT;

    return INSTRUCTION_NONE;
}

/**
*  解析方向词返回枚举
 * @param
 * @return 
 */
ORIE_CASE parse_orientation(const char* input) {
	
    if (input == NULL) return ORIENTATION_NONE;

    if (strcmp(input, "NW") == 0) return ORIENTATION_NORTHWEST;
    if (strcmp(input, "E")  == 0) return ORIENTATION_EAST;
    if (strcmp(input, "N")  == 0) return ORIENTATION_NORTH;
    if (strcmp(input, "S")  == 0) return ORIENTATION_SOUTH;
    if (strcmp(input, "W")  == 0) return ORIENTATION_WEST;
    if (strcmp(input, "NE") == 0) return ORIENTATION_NORTHEAST;
    if (strcmp(input, "SE") == 0) return ORIENTATION_SOUTHEAST;
    if (strcmp(input, "SW") == 0) return ORIENTATION_SOUTHWEST;
    if (strcmp(input, "NO") == 0) return ORIENTATION_NONE; // 特殊情况

    return ORIENTATION_NONE;
}

/**
*  解析方向词返回角度
 * @param
 * @return 
 */
float parse_orientation_to_angle(const char* input)
{
	if (strcmp(input, "N") == 0) return 0.0f;
    else if (strcmp(input, "NE") == 0) return 45.0f;
    else if (strcmp(input, "E") == 0) return 90.0f;
    else if (strcmp(input, "SE") == 0) return 135.0f;
    else if (strcmp(input, "S") == 0) return 180.0f;
    else if (strcmp(input, "SW") == 0) return 225.0f;
    else if (strcmp(input, "W") == 0) return 270.0f;
    else if (strcmp(input, "NW") == 0) return 315.0f;
    else return -1.0f;  // 无效输入，返回错误
}
