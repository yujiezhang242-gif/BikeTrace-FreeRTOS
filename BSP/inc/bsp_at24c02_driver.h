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
 #ifndef __BSP_AT24C02_DRIVER_H__
 #define __BSP_AT24C02_DRIVER_H__
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private define ------------------------------------------------------------*/

/* 
 * AT24C02 2kb = 2048bit = 2048/8 B = 256 B
 * 32 pages of 8 bytes each
 *
 * Device Address
 * 1 0 1 0 A2 A1 A0 R/W
 * 1 0 1 0 0  0  0  0 = 0XA0
 * 1 0 1 0 0  0  0  1 = 0XA1 
 */
/* AT24C01/02每页有8个字节 
 * AT24C04/08A/16A每页有16个字节 
 */
#define EEPROM_DEV_ADDR   0xA0  /* 24xx02的设备地址 */
#define EEPROM_PAGE_SIZE    8     /* 24xx02的页面大小 */
#define EEPROM_SIZE        256     /* 24xx02总容量 */

typedef struct
{
	uint8_t (*init)(void);
	uint8_t (*checkOk)(void);
	uint8_t (*readBytes)(uint8_t *_pReadBuf, uint16_t _usAddress, uint16_t _usSize);
	uint8_t (*writeBytes)(uint8_t *_pWriteBuf, uint16_t _usAddress, uint16_t _usSize);
	void    (*erase)(void);
} bsp_at24c02_driver_t;

extern bsp_at24c02_driver_t at24c02;

uint8_t ee_Test(void);

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

#endif /* __BSP_AT24C02_DRIVER_H__ */

