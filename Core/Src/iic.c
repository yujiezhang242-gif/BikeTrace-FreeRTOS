/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file iic.c
 * 
 * @par dependencies 
 * - iic.h
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
#include "iic.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void iic_init(void);
static void iic_start(void);
static void iic_stop(void);
static void iic_send_byte(uint8_t data);
static uint8_t iic_wait_ack(int16_t timeout);
static void iic_ack(void);
static void iic_nack(void);
static uint8_t iic_read_data(void);

static void iic_write(uint8_t slave_addr, uint8_t *data, uint16_t data_size);
static void iic_read(uint8_t slave_addr, uint8_t* data, uint16_t data_size);

core_iic_handle_t iic = 
{
	.init  = iic_init,
	.ctrl = 
	{
		.start = iic_start,
		.stop = iic_stop,
		.waitAck = iic_wait_ack,
		.ack = iic_ack,
		.nack = iic_nack,
		.sendbyte = iic_send_byte,
		.readbyte = iic_read_data,
	},
	.write = iic_write,
	.read  = iic_read,
};
/* Private functions ---------------------------------------------------------*/

void iic_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	/*Configure GPIO pin : PB9 */
	GPIO_InitStruct.Pin = I2C_SCL_PIN | I2C_SDA_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(I2C_GPIO_PORT, &GPIO_InitStruct);

	//I2C_SCL_H();
	//I2C_SDA_H();

	iic_stop();
}

void iic_delay(void)
{
	uint8_t i;
	for(i = 0; i < 60; i++);
}

void iic_start(void)
{
	I2C_SCL_H();
	//I2C_DELAY();
	I2C_SDA_H();
	I2C_DELAY();
	I2C_SDA_L();
	I2C_DELAY();
	I2C_SCL_L();
	//I2C_DELAY();

	I2C_DELAY();
}

void iic_stop(void)
{
	I2C_SDA_L();
	I2C_SCL_H();
	I2C_DELAY();
	I2C_SDA_H();
	//I2C_DELAY();

	I2C_DELAY();
}

/* [注]：adc - 1表示应答 - 0表示器件未应答 */
uint8_t iic_wait_ack(int16_t timeout)
{
	uint8_t ack = 0;
	/* release */
//	I2C_SDA_H();
//	I2C_DELAY();
	I2C_SDA_H();
	I2C_DELAY();

	I2C_SCL_H();
	I2C_DELAY();

#if 0
	do{
		timeout--;
		Delay_Us(2);
	} while((I2C_READ_SDA())&&(timeout>=0));
	
	if (timeout > 0)
	{
		ack = 1;
	}
#endif

	if (I2C_READ_SDA() == 0)
	{
		ack = 1;
	}	

	I2C_SCL_L();
	I2C_DELAY();

	return ack;
}

void iic_send_byte(uint8_t data)
{
	int8_t i;
	
	for (i = 0; i < 8; i++)
	{
		if (data & 0x80)
		{
			I2C_SDA_H();
		}
		else
		{
			I2C_SDA_L();
		}

		I2C_DELAY();

		I2C_SCL_H();
		I2C_DELAY();
		I2C_SCL_L();
		I2C_DELAY();

		if (i == 7)
		{
			I2C_SDA_H();
		}

		data <<= 1;
		I2C_DELAY();
	}
	/* release */
	
}

void iic_ack(void)
{
	I2C_SDA_L();
	//I2C_DELAY();

	I2C_DELAY();
	I2C_SCL_H();
	I2C_DELAY();
	/* release */

	I2C_SCL_L();

	//I2C_DELAY();
	//I2C_SDA_H();
	//I2C_DELAY();

	I2C_DELAY();
	I2C_SDA_H();
	I2C_DELAY();
}

void iic_nack(void)
{
	I2C_SDA_H();
	//I2C_DELAY();

	I2C_DELAY();

	I2C_SCL_H();
	I2C_DELAY();
	/* release */
	I2C_SCL_L();
	//I2C_DELAY();

	I2C_DELAY();
}

uint8_t iic_read_data(void)
{
	int8_t i;
	
	/* 读取数据前需要拉高SDA */
	//I2C_SDA_H();

	uint8_t data = 0;
	for (i=0; i < 8; i++)
	{
		data <<= 1;
		I2C_SCL_H();
		I2C_DELAY();
		if (I2C_READ_SDA())
			data++;
		I2C_SCL_L();
		I2C_DELAY();
	}
	
	return data;
}

/**
  * @brief  写入data_size个字节数据
  * @param  slave_addr 从机地址
  * @param  data 数据首地址
  * @param  data_size 数据长度
  * @retval 无
  */
void iic_write(uint8_t slave_addr, uint8_t *data, uint16_t data_size)
{
	iic_start();
	iic_send_byte(slave_addr | I2C_WRITE_BIT);
	if (iic_wait_ack(100) == 0)
	{
		return;
	}
	
	while(data_size--)
	{
		iic_send_byte(*data++);
		if (iic_wait_ack(100) == 0)
		{
			return;
		}
	}
	
	iic_stop();
}

/**
  * @brief  读取data_size个字节数据
  * @param  slave_addr 从机地址
  * @param  data 数据首地址
  * @param  data_size 数据长度
  * @retval 无
  */
void iic_read(uint8_t slave_addr, uint8_t* data, uint16_t data_size)
{
    iic_start();
    iic_send_byte(slave_addr | I2C_READ_BIT);
	
	if (iic_wait_ack(100) == 0)
	{
		return;
	}
    
    /* 如果data_size是1，最后一个字节（减1后是0了），则设置回复 NACK */
    while(data_size--) {
		*data++ = iic_read_data();

		if(data_size == 0)
		{
			iic_nack();
		}
		else
		{
			iic_ack();
		}
	}

	iic_stop();
}
