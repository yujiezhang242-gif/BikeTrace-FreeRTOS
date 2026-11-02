/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file at24cxx_driver.h
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
 #ifndef __AT24CXX_DRIVER_H
 #define __AT24CXX_DRIVER_H
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "iic.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define EEPROM_SIZE  													  (256)
#define PAGE_SIZE  														    (8)
#define AT24C02_WADDR               									 (0xA0)
#define AT24C02_RADDR               									 (0xA1)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
uint8_t AT24C02_init(void);
uint8_t AT24C02_WriteByte(uint8_t addr, uint8_t wdata);
uint8_t AT24C02_WritePage(uint8_t addr, uint8_t *wdata);
uint8_t AT24C02_WriteData(uint8_t addr, uint8_t *wdata, uint16_t datalen);
uint8_t AT24C02_ReadData(uint8_t addr, uint8_t *rdata, uint16_t datalen);

uint8_t AT24C02_Test(void);
/* Private functions ---------------------------------------------------------*/

#endif //__AT24CXX_DRIVER_H
