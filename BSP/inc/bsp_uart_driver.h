/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_uart_driver.h
 * 
 * @par dependencies 
 * - usart.h
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
 #ifndef __BSP_UART_DRIVER_H__
 #define __BSP_UART_DRIVER_H__
/* Includes ------------------------------------------------------------------*/
#include "usart.h"
#include "main.h"
/* Private typedef -----------------------------------------------------------*/

typedef struct
{
	void (*start)          				   (void);
	void (*sendByte) 			   (uint8_t data);
	void (*sendArr) (uint8_t *data, uint16_t len);
	void (*sendString)                (char *str);
	uint8_t (*getRxData)                   (void);
} bsp_uart_driver_t;

extern bsp_uart_driver_t u2;
extern bsp_uart_driver_t u3;

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
void debug_uart_init(void);
/* Private functions ---------------------------------------------------------*/

#endif /* __BSP_UART_DRIVER_H__ */

