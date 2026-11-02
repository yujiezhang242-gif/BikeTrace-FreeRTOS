/******************************************************************************
 * Copyright (C) 2025 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file at_ec600n_driver.h
 * 
 * @par dependencies 
 * - main.h 
 * - at_ec600n_handler.h
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

#ifndef __AT_EC600N_DRIVER_H
#define __AT_EC600N_DRIVER_H

#include "main.h"
#include "at_ec600n_handler.h"

#define AT_COMMAND_MAX_LEN 128

#define EC600N_CONNECTED_SERVER       1
#define EC600N_NOT_CONNECTED_SERVER   0

typedef struct
{
	void (*init)(void);
	AT_RESULT (*connectServer)(const char *ip, int port);
	AT_RESULT (*disconnectServer)(void);
	int (*getconnectStatus)(void);
	void (*send)(const char *data, int len);
	int (*recv)(char *buffer, int size);
} bsp_ec600n_tcp_driver_t;

extern bsp_ec600n_tcp_driver_t ec600n_tcp;




#endif //__AT_EC600N_DRIVER_H
