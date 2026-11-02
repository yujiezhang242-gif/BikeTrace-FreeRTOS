/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file screen.h
 * 
 * @par dependencies 
 * - main.h
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
#ifndef __SCREEN_H_
#define __SCREEN_H_
 
/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "network.h"
/* Private typedef -----------------------------------------------------------*/

#define ARROW_UP         0  // б№ (0бу)
#define ARROW_UP_RIGHT   1  // иJ (45бу)
#define ARROW_RIGHT      2  // б· (90бу)
#define ARROW_DOWN_RIGHT 3  // иK (135бу)
#define ARROW_DOWN       4  // б¤ (180бу ╗Є -180бу)
#define ARROW_DOWN_LEFT  5  // иL (-135бу)
#define ARROW_LEFT       6  // б√ (-90бу)
#define ARROW_UP_LEFT    7  // иI (-45бу)

typedef struct
{
	void (*init)                        (void);
	void (*clear)                       (void);
	void (*idle_show)  (char* time, int gps_enable);
	void (*path_show)  (char *time, uint32_t remain_m, INST_CASE instcase, \
													uint8_t arrow_index);
	void (*info_1_show)(char* time, float speed_kmh, float distance_km, \
									float total_seconds, int running_state);
	void (*info_2_show)(float max_speed_kmh, float average_speed_kmh, \
					float kcal, float temp, float humi, int running_state);
	void (*reload_show)(void);
} app_screen_t;

extern app_screen_t screen;


/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

#endif /* __SCREEN_H_ */
