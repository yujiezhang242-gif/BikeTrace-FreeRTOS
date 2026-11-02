/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file delay.h
 * 
 * @par dependencies 
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
#ifndef __IIC_H
#define __IIC_H
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "delay.h"
/* Private typedef -----------------------------------------------------------*/

static void iic_start(void);
static void iic_stop(void);
static void iic_send_byte(uint8_t data);
static uint8_t iic_wait_ack(int16_t timeout);
static void iic_send_byte(uint8_t data);
static uint8_t iic_wait_ack(int16_t timeout);
static void iic_ack(void);
static void iic_nack(void);
static uint8_t iic_read_data(void);
void iic_delay(void);

typedef struct
{
	void    (*start)(void);
	void    (*stop)(void);
	uint8_t (*waitAck)(int16_t timeout);
	void    (*ack)(void);
	void    (*nack)(void);
	void    (*sendbyte)(uint8_t data);
	uint8_t (*readbyte)(void);
} core_iic_driver_t;

typedef struct
{
	void (*init)(void);
	core_iic_driver_t ctrl;
	void (*write)(uint8_t slave_addr, uint8_t *data, uint16_t data_size);
	void (*read)(uint8_t slave_addr, uint8_t *data, uint16_t data_size);
} core_iic_handle_t;

extern core_iic_handle_t iic;

/* Private define ------------------------------------------------------------*/
#define I2C_WRITE_BIT								 						(0x00)
#define I2C_READ_BIT                     							  	    (0x01)

#define I2C_BIT(x)                                                       (1U<<(x))
#define I2C_ACK                                                                (1)
#define I2C_NACK                                                               (0)

#define I2C_GPIO_PORT             				   						   (GPIOB)
#define I2C_SCL_PIN                                  				  (GPIO_PIN_6)
#define I2C_SDA_PIN                                                   (GPIO_PIN_7)

#define I2C_SCL_H()    HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SCL_PIN, GPIO_PIN_SET)
#define I2C_SCL_L()  HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SCL_PIN, GPIO_PIN_RESET)

#define I2C_SDA_H()    HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SDA_PIN, GPIO_PIN_SET)
#define I2C_SDA_L()  HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SDA_PIN, GPIO_PIN_RESET)

#define I2C_READ_SDA()   		      HAL_GPIO_ReadPin(I2C_GPIO_PORT, I2C_SDA_PIN)

#define I2C_DELAY()                       							   iic_delay()

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
#endif 
