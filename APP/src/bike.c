/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bike.c
 * 
 * @par dependencies 
 * - bike.h
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
#include "bike.h"
#include "utils.h"
#include "screen.h"
#include "flash_index.h"
#include "flash_data.h"
#include "network.h"

#include "cJSON.h"
#include <math.h>
#include <stdbool.h>

#include "bsp_gps_driver.h"
#include "bsp_key_driver.h"
#include "bsp_aht21_driver.h"
#include "bsp_timer_driver.h"
#include "bsp_at24c02_driver.h"
#include "at_ec600n_tcp_driver.h"
#include "bsp_hmc5883l_driver.h"

/* Private define ------------------------------------------------------------*/

#define ITEM_INTERVAL_S                 (10)      /* 写入记录的时间间隔 */
#define UPLOAD_INTERVAL_S               (2)      /**< 上传到服务器的时间间隔 */
#define TIME_SYNC_INTERVAL_S            (180)      /**< 上传到服务器的时间间隔 */
#define TCP_SERVER_IP                   "39.108.115.184" //"47.113.179.18"
#define TCP_SERVER_PORT                 (8081)        //(8081)

uint32_t timestamp_local = 0;                      // 本地时间戳
													  
/* Private typedef -----------------------------------------------------------*/

typedef struct 
{
	bike_status_t 				   status;     /* 骑行状态 */
	screen_seq_t 		   current_screen;     /* 当前屏幕 */
	float               total_distance_km;     /* 骑行距离，单位：千米 */
    uint32_t               total_time_sec;     /* 骑行时间，单位：秒 */
    float               		speed_kmh;     /* 实时速度，单位：千米每小时 */
	float               	max_speed_kmh;     /* 最大时速 */
	
    float               average_speed_kmh;     /* 平均时速 */
    float               			 humi;     /* 实时湿度 */
    float               			 temp;     /* 实时温度 */
    float               	 average_humi;     /* 平均湿度 */
    float                	 average_temp;     /* 平均温度 */
    float              				 kcal;     /* 消耗卡路里 */
    uint8_t             	   reset_data;     /* 是否重置统计变量的标志（用于清除总统计量或平均值） */
	
	uint8_t 			 data_export_list;     /* 列出保存的数据项 */
	uint8_t 			    data_export_n;     /* 导出数据项编号 */
											  
} bike_info_t;                                
											 
static bike_info_t bike;

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

void bike_reset(void);

static void bike_data_list(void);
static int bike_data_export(void);

/* Private functions ---------------------------------------------------------*/

/* 传递骑行一系列参数 */
float bike_get_distance_km(void)
{
    return bike.total_distance_km;
}
char* bike_get_time_str(void)
{
    return gps.get_time_str();
}
float bike_get_max_speed_kmh(void)
{
    return bike.max_speed_kmh;
}
float bike_get_average_speed_kmh(void)
{
    return bike.average_speed_kmh;
}
float bike_get_kcal(void)
{
    return bike.kcal;
}
float bike_get_average_humi(void)
{
    return bike.average_humi;
}
float bike_get_average_temp(void)
{
    return bike.average_temp;
}
float bike_get_temp(void)
{
	return bike.temp;
}
float bike_get_humi(void)
{
	return bike.humi;
}
float bike_get_speed_kmh(void)
{
	return bike.speed_kmh;
}
uint32_t bike_get_total_time_sec(void)
{
	return bike.total_time_sec;
}
bike_status_t bike_get_status(void)
{
	return bike.status;
}

// 返回本地时间
uint32_t bike_get_local_time_val(void)
{
	return timestamp_local;
}

/* *************** add code ***************** */
/* 传递骑行一系列参数 */
void bike_set_distance_km(uint16_t total_distance_km)
{
    bike.total_distance_km = uint16_to_float(total_distance_km);
}
void bike_set_total_time_sec(uint32_t last_timestamp)
{
	uint16_t current_table = get_table_cnt();
    uint32_t create_timestamp = get_table_n_created_timestamp(current_table);
	
	bike.total_time_sec = last_timestamp - create_timestamp;
	//printf("restore bike.total_time_sec = [%d]\r\n", bike.total_time_sec);
}
void bike_set_max_speed_kmh(uint16_t max_speed_kmh)
{
    bike.max_speed_kmh = uint16_to_float(max_speed_kmh);
}
void bike_set_average_speed_kmh(uint16_t average_speed_kmh)
{
    bike.average_speed_kmh = uint16_to_float(average_speed_kmh);
}
void bike_set_kcal(float kcal)
{
    bike.kcal = kcal;
}
void bike_set_average_humi(uint16_t average_humi)
{
    bike.average_humi = uint16_to_float(average_humi);
}
void bike_set_average_temp(int16_t average_temp)
{
    bike.average_temp = int16_to_float(average_temp);
}

// 设置本地时间
void bike_set_local_time_val(uint32_t time_set_val)
{
	timestamp_local = time_set_val;
}

/* *************** add code ***************** */

// 计算相差距离
static float calculate_relative_angle(int16_t target_angle, int16_t current_angle) {
    float diff = target_angle - current_angle;
    diff = fmod(diff + 180.0f, 360.0f) - 180.0f;  // 归一化到-180~180
    return diff;
}

// 获取箭头宏定义
int get_arrow_macro(float rel_angle) {

    // 分段判断，添加±22.5°的阈值（45°/2），以分成8个区间
    if (rel_angle >= -22.5 && rel_angle < 22.5) return ARROW_UP;
    else if (rel_angle >= 22.5 && rel_angle < 67.5) return ARROW_UP_RIGHT;
    else if (rel_angle >= 67.5 && rel_angle < 112.5) return ARROW_RIGHT;
    else if (rel_angle >= 112.5 && rel_angle < 157.5) return ARROW_DOWN_RIGHT;
    else if (rel_angle >= 157.5 || rel_angle < -157.5) return ARROW_DOWN;  // 包括-180~ -157.5 和157.5~180
    else if (rel_angle >= -157.5 && rel_angle < -112.5) return ARROW_DOWN_LEFT;
    else if (rel_angle >= -112.5 && rel_angle < -67.5) return ARROW_LEFT;
    else if (rel_angle >= -67.5 && rel_angle < -22.5) return ARROW_UP_LEFT;

    return ARROW_UP;  // 默认
}

static void bike_path_plan_reload_request(void)
{
	// 1. 清除路径规划完成标志位
	clear_ride_path_done_flag();
	
	// 2. 释放上次路径规划内存空间
	reset_path_memory();
	
	// 3. 发送路径规划请求给TCP
	bike_path_plan_request();
}

static void screen_path_handle(char *time, float distance_km)
{
    static uint8_t first_path = 0;
    static uint8_t step_switch = 1;
    static uint16_t step_n = 0;
    static uint32_t last_distance = 0;
    static int32_t remain_m = 0; // 直接使用静态 remain_m 保存剩余距离
    static INST_CASE instcase;
    static char *inst = NULL, *orie = NULL;
    static uint32_t step_distance;
	static uint32_t err_distance = 0;
	static uint8_t scree_refresh = 0;
    
    uint32_t distance_m = distance_km * 1000; // 转换为米
	uint32_t traveled_distance;
	
	// 路径规划未完成，返回/或者显示加载路径中..[用于路径偏移重新规划]
	if (!check_ride_path_gener_done())
	{
		screen.reload_show();
		scree_refresh = 1;
		return;
	}
	
	if (scree_refresh)
	{
		scree_refresh = 0;
		screen.clear();
	}
		

    // 初始化第一步路径
    if (!first_path)
    {
        get_step_n_info(step_n, &inst, &orie, &step_distance);
        remain_m = step_distance; // 初始化剩余距离为当前步骤总距离
        first_path = 1;
    }

//    printf("step %u: inst = [%s], orie = [%s], step_distance = [%u], remain_m = [%u]\r\n", 
//           step_n, inst, orie, step_distance, remain_m);

    // 切换到下一步
    if (step_switch)
    {
        //distance_record = distance_m; // 记录当前总距离作为起点
        instcase = parse_instruction(inst);
        remain_m = step_distance; // 重置剩余距离为新步骤的总距离
        printf("instcase = [%d]\r\n", instcase);
        step_switch = 0;
    }

   // 获取当前方向并计算相对角度
    float current_angle = hmc5883l.getAngle();
	
	// 获取目标方向角度
    float target_angle = parse_orientation_to_angle(orie);
    if (target_angle < 0) // 无效方向
    {
		// 暂定为当前方向作为目标方向，可以优化为上一个路径与指示的角度配合
        target_angle = current_angle;
    }
 
    float rel_angle = calculate_relative_angle(target_angle, current_angle);

    // 判断方向是否正确
    int arrow = get_arrow_macro(rel_angle);
    bool is_direction_correct = (arrow == ARROW_UP) || \
						(arrow == ARROW_UP_RIGHT) || (arrow == ARROW_UP_LEFT);

	// 记录位移距离
	traveled_distance = distance_m - last_distance;
	
	// 防止位移量过大（测试用，实际需要调整）
	if (traveled_distance > 15)
		traveled_distance = 10;
	
    // 计算剩余距离
    if (is_direction_correct)
    {
		err_distance = 0;
		// 方向正确，更新剩余距离
        remain_m -= traveled_distance;
    }
    else
    {
		err_distance += traveled_distance;
        // 方向不正确，增大剩余距离
		remain_m += traveled_distance;
        printf("Direction incorrect, angle diff: %.2f°, maintaining remain_m: %u\r\n", 
               rel_angle, remain_m);
    }
	
	// 更新本次距离记录
	last_distance = distance_m;
	
	printf("err_dist = [%d]\r\n", err_distance);
		
	if (err_distance > 100)
	{
		// 清空错误路径累计值
		err_distance = 0;
		
		// 清空相关静态变量
		step_n = 0;
		first_path = 0;
		
		// 申请重新路径规划
		bike_path_plan_reload_request();
		
		// 刷新屏幕
		screen.clear();
		
		return;
		// 返回，释放错过路径内存，不允许访问已经释放内存的路径信息
	}

    // 如果剩余距离小于2米且未到达最后一步，切换到下一步
    if (remain_m < 5 && step_n < get_step_num())
    {
        step_n++;
        get_step_n_info(step_n, &inst, &orie, &step_distance);
        step_switch = 1;
        remain_m = step_distance; // 重置剩余距离
        printf("Switching to step %u, new step_distance: %u\r\n", step_n, step_distance);
    }
	
	
    // 显示路径信息
    screen.path_show(time, remain_m, instcase, arrow);
}

void test(char *time, float distance_km)
{
	screen_path_handle(time, distance_km);
}


static void bike_screen_show(void)
{
    char time_str[18] = {0};
    strcpy(time_str, gps.get_time_str());

    if (bike.current_screen == SCREEN_IDEL) {
        screen.idle_show(&time_str[3], gps.get_valid_val());
    } else if (bike.current_screen == SCREEN_1) {
        screen.info_1_show(&time_str[3], bike.speed_kmh, bike.total_distance_km,
             bike.total_time_sec, bike.status);
    } else if(bike.current_screen == SCREEN_2){
        screen.info_2_show(bike.max_speed_kmh, bike.average_speed_kmh, 
            bike.kcal, bike.temp, bike.humi, bike.status);
    } else if(bike.current_screen == SCREEN_PATH) {
		// 处理路径信息，路径界面
		screen_path_handle(&time_str[3], bike.total_distance_km);
	}
}

static void bike_screen_change(void)
{
    if (bike.current_screen == SCREEN_1) {
        bike.current_screen = SCREEN_2;
        screen.clear();
    } else if (bike.current_screen == SCREEN_2) {
        bike.current_screen = SCREEN_PATH;
        screen.clear();
    } else if (bike.current_screen == SCREEN_PATH) {
		bike.current_screen = SCREEN_1;
		screen.clear();
	}
}

/* 当前停止，切换至欢迎状态 */
static void bike_switch_idle_status_handle(void)
{
	screen.clear();
	
	// [关键] - 重置骑行参数/状态
	bike_reset();                    
}

/* 当前空闲，切换至骑行状态 */
void bike_switch_ride_status_handle(Restore_Flag restore_flag)
{
	screen.clear();
	bike.status = RIDE;              
	bike.current_screen = SCREEN_PATH;
	LED_RIDE_ON();
	
	/* ====== 目的地经纬度 ====== */
	
//	HAL_Delay(300);
//	
//	// 链接TCP网络
//	ec600n_tcp.connectServer(TCP_SERVER_IP, TCP_SERVER_PORT);
	
	// [关键] - 添加创建表的时间戳（非恢复标志位则创建时间戳）
	if (restore_flag != IS_RESTORE)
		set_table_created_timestamp(time_str_to_timestamp(gps.get_time_str()));
}

/* 当前骑行，切换至停止状态 */
static void bike_switch_stop_status_handle(void)
{
	bike.status = STOP;    
	LED_RIDE_OFF();	
	
	// 关闭TCP链接
	//ec600n_tcp.disconnectServer();
	
	// [关键] - 将骑行未写入的item写入
	save_item(gps.get_latitude(), gps.get_longitude(), bike.speed_kmh, bike.status);
}


static void bike_status_change(void)
{
    if (bike.status == IDLE && gps.get_valid_val() ) { /* 在GPS有效前，不响应按键 */
        
		/* 切换至骑行状态 */
		bike_switch_ride_status_handle(NOT_RESTORE);
		
    } else if (bike.status == RIDE) {
        
		/* 切换至停止状态 */
		bike_switch_stop_status_handle();
		
    } else if (bike.status == STOP) {
        
		/* 切换至空闲状态 */
		bike_switch_idle_status_handle();
    }
	
    printf("bike state:%d\r\n", bike.status);
}

void bike_handle_key(void)
{
	/* 处理启停按键的事件 */
	if (key_on_off.getkeyStatus() == KEY_SHORT_PRESS)
	{
		bike_status_change();
	}
	
	/* 处理切换屏幕按键的事件 */
	if (key_switch.getkeyStatus() == KEY_SHORT_PRESS)
	{
		bike_screen_change();
	}
}

/* 运行后，1秒调用一次 */
static void bike_calc_ride_data(void)
{
	static uint8_t once_flag = 0;
	
	/* 获取GPS传递的速度（节） */
    float speed_kt = gps.get_speed_kn();

	/* 根据速度节计算距离（公里） */
    float distance_km = calc_distance_m(speed_kt, 1) / 1000;
    
	/* 计算速度km*h */
    bike.speed_kmh = calc_speed_kmh(speed_kt);
	
	/* 计算总里程 */
	if (get_restore_flag_status() == IS_RESTORE && !once_flag)
	{
		/* 如果是异常状态，需要补上之前的总里程 */
		distance_km += bike.total_distance_km;
		once_flag = 1;
	}	
    bike.total_distance_km = calc_total_distance_km(distance_km, bike.reset_data);
	
	/* 计算最大速度 */
    bike.max_speed_kmh = calc_max_speed_kmh(bike.speed_kmh, bike.reset_data);
	
	/* 计算平均速度 */
    bike.average_speed_kmh = calc_average_speed(bike.total_distance_km, bike.total_time_sec);
	
	/* 计算卡路里 */
    bike.kcal = calc_kcal(bike.total_distance_km);
    
    /* 读取温湿度数据 */
	aht21.parse_humi_temp();
    bike.humi = aht21.getHumiVal();
	bike.temp = aht21.getTempVal();
	
	/* 计算平均温湿度数据 */
    calc_average_humi_temp(bike.humi, bike.temp,
        &bike.average_humi, &bike.average_temp, bike.reset_data);

	/* reset_data标志已经使用，清除标志位 */
    if(bike.reset_data)
        bike.reset_data = 0;
}

 /**********************************************************************************
  * 函数功能: 根据tim中断标志位（1s）触发一次更新
  * 输入参数: 无
  */
static void bike_update(void)
{
    if (tim.getTimFlag() == 0)
		return;
	
	tim.clearTimFlag();
	
	static uint32_t sec = 0;
	static uint8_t once_flag = 0;
	
//	/* 如果在异常断电状态，等待GPS有效信号恢复骑行状态，仅执行一次 */
//	if (get_restore_flag_status() == IS_RESTORE && gps.get_valid_val() && !once_flag)
//	{
//		restore_last_ride_status();
//		once_flag = 1;
//	}
	
	/* 未连接尝试连接TCP服务器 */
	if (!ec600n_tcp.getconnectStatus())
	{
		ec600n_tcp.connectServer(TCP_SERVER_IP, TCP_SERVER_PORT);
		//bike_cloud_time_request();
	}
	
	
	/* 未骑行收到路径规划内容，闪烁提醒 */
	if (check_ride_path_gener_done() && bike.status == IDLE)
	{
		LED_RIDE_TOGGLE();
	}
	
	if (sec % TIME_SYNC_INTERVAL_S == 0 && ec600n_tcp.getconnectStatus())
		bike_cloud_time_request();
	
	/* 大于0表示已经云端同步过 */
	if (timestamp_local)
	{
		timestamp_local++;
	}

	sec++;

	/* 如果在骑行状态，更新骑行参数 */
    if (bike.status == RIDE) 
	{
        bike.total_time_sec++;

        bike_calc_ride_data();
		
		/* 每隔2秒，保存一条经纬速度 */
		if (sec % UPLOAD_INTERVAL_S == 0 && ec600n_tcp.getconnectStatus() && timestamp_local)
		{
			bike_cloud_upload_data();
		}
        
        /* 每隔10秒，保存一条经纬速度 */
        if (sec % ITEM_INTERVAL_S == 0) 
		{
            save_item(gps.get_latitude(), gps.get_longitude(), bike.speed_kmh, bike.status);
        }
		
		// 状态指示
		LED_GPS_TOGGLE();
    }

	/* 刷新屏幕 */
    bike_screen_show();
	
    //调试时开启
    //bike_print();
}

void bike_handle(void)
{
//	/* 检测按键 */
//	bike_handle_key();

//	/* 骑行参数更新 */
//	bike_update();
	
	/* 骑行数据接收 */
	bike_cloud_recv_data();

	/* 处理骑行数据请求/输出 */
	bike_data_list();
	bike_data_export();
	
}

void bike_reset(void)
{
    memset(&bike, 0, sizeof(bike_info_t));

    bike.total_distance_km = 0.0;
    bike.total_time_sec = 0;
    bike.speed_kmh = 0.0;
    bike.max_speed_kmh = 0.0;

    bike.status = IDLE;
    bike.current_screen = SCREEN_IDEL;

    bike.reset_data = 1;

    set_first_write_flag();
}

void bike_init(void)
{
    memset(&bike, 0, sizeof(bike_info_t));
	
	/* 初始化OLED */
	screen.init();
	
	if (at24c02.init() == 0)
	{
		/* 没有检测到EEPROM */
		printf("no detect the EEPROM!\r\n");
	}
	
	ec600n_tcp.init();
	
	init_flash();
    
    bike_reset();
	
	//write_write_flag_to_at24();
}

// ********************************* 输出骑行数据 *********************************** //

 /**********************************************************************************
  * 函数功能: [调试]列出目前已有的骑行数据
  * 输入参数: 无
  */
static void bike_data_do_list(void)
{
    int i;
    char time_str[32] = {0};
    int cnt = get_table_cnt();
    
    printf("=====================================================\r\n");
    if(cnt > 0) {
        printf("There are [%d] riding data in flash, select number to export [1-%d]\r\n", cnt, cnt);
        for(i = 1; i <= cnt; i++) {
            printf("riding [%d] : created at %s\r\n", 
                i, timestamp_to_time_str(get_table_n_created_timestamp(i), time_str));
        }
    } else {
        printf("There is [0] riding data in flash\r\n");
    }
    printf("=====================================================\r\n");
}

 /**********************************************************************************
  * 函数功能: 判断是否有输出指令，执行输出骑行列表
  * 输入参数: 无
  */
static void bike_data_list(void)
{
    if(bike.data_export_list == 1) {
        bike.data_export_list = 0;
        bike_data_do_list();
    }
}

 /**********************************************************************************
  * 函数功能: 骑行数据列出请求 - 由UART1中断接收到"get data\n"指令调用
  * 输入参数: 无
  */
void bike_data_list_request(void)
{
    bike.data_export_list = 1;
}

 /**********************************************************************************
  * 函数功能: 输出骑行次序数据
  * 输入参数: 无
  */
static int bike_data_export(void)
{    
    if(bike.data_export_n != 0) {
        if (bike.status != IDLE) { /* 只能在空闲时导出数据 */
            bike.data_export_n = 0;
            return -1;
        }
        
        print_table_n(bike.data_export_n);
        bike.data_export_n = 0;
    }
    return 0;
}

 /**********************************************************************************
  * 函数功能: 输出骑行次序数据请求 - 由UART1中断接收到"n\n"指令调用
  * 输入参数: 无
  */
void bike_data_exprot_request(char* cmd)
{
    int n;

    if(sscanf(cmd, "%d", &n) == 1) {
        if(n >= 1 && n <= 16)
            bike.data_export_n = n;
        else
            printf("invalid cmd [%d]\r\n", n);
    }
}

