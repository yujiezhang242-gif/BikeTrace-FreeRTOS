/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_aht21_driver.c
 * 
 * @par dependencies 
 * - bsp_aht21_driver.h
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
#include "bsp_aht21_driver.h"
#include "bsp_aht21_handle.h"
#include "bsp_uart_driver.h"
#include "iic.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
      					
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

static float humi = 0.0;
static float temp = 0.0;

/* Private function prototypes -----------------------------------------------*/
static void aht10_reset(void);
static int aht10_read(void);
static void aht21_init(void);
static float aht21_get_humi(void);
static float aht21_get_temp(void);

bsp_aht21_driver_t aht21 =
{
	.init = aht21_init,
	.parse_humi_temp = aht10_read,
	.getHumiVal = aht21_get_humi,
	.getTempVal = aht21_get_temp,
};
/* Private functions ---------------------------------------------------------*/
/**
  * @brief  ??¦ΛAHT10
  * @param  ??
  * @retval ??
  */
void aht10_reset(void)
{
    /* 1011 1010 */
    uint8_t reset = 0xBA;
    /* ???????????¦Λ */
    iic.write(AHT21_ADDRESS, &reset, 1);
	
	/* ?????????¦Λ?????? 20 ???? */
    Delay_Ms(20);
}

static void aht21_init(void)
{
	Delay_Ms(10);
	
	uint8_t reg[3] = {0xBE, 0x04, 0x00};
	
	/* ????§µ????? */
	iic.write(AHT21_ADDRESS, reg, 3);

	Delay_Ms(10);
	
	// ??§µ?????
	aht21.parse_humi_temp();
}

static uint8_t aht21_check_status(void)
{
	uint8_t checkByte = 0x71, statusByte;

	/* ????§µ????? */
	iic.write(AHT21_ADDRESS, &checkByte, 1);
	
	iic.read(AHT21_ADDRESS, &statusByte, 1);
	
	return statusByte;

}
/**
  * @brief  ???AHT10????
  * @param  *humi ????????
  * @param  *temp ??????¦Λ???????
  * @retval 0 - ????????-1 - ??????
  */
int aht10_read(void)
{
    uint8_t humi_temp[6];/* ??????????????????? */
    /*  10101100, 00110011, 00000000 */
    uint8_t measure[3] = {0xAC, 0x33, 0x00};
	
	uint8_t status = aht21_check_status();
	
	if ((status |= 0x18) != 0x18)
	{
		aht21_init();
		Delay_Ms(50);
		return -1;
	}

    /* ????????????? */
    iic.write(AHT21_ADDRESS, measure, 3);
    /* ????????????????? 75 ??????????????????????υτ?????????? */
    Delay_Ms(100);
    /* ???6????? */
    iic.read(AHT21_ADDRESS, humi_temp, 6);
	
	//u3.sendArr(humi_temp,6);
    
    /* 0x08(00001000)????????? 3 ¦Λ(§µ????¦Λ)?????0???????¦Δ§µ??????? */
    if( (humi_temp[0] & 0x08) == 0 ) {
        aht10_reset();
        return 0;
    } else if ( (humi_temp[0] & 0x80) == 0 ) {/* 0x80(10000000)?????????7¦Λ(?????)??0 ?????????? */
        uint32_t SRH = (humi_temp[1]<<12) | (humi_temp[2]<<4) | (humi_temp[3]>>4);  /* ?????????? */
        uint32_t ST = ((humi_temp[3]&0x0f)<<16) | (humi_temp[4]<<8) | humi_temp[5];/* ?????????? */

//        *humi = (SRH * 100.0) / 1024.0 / 1024;     /* ?????????????????? */
//        *temp = (ST * 200.0) / 1024.0 / 1024 - 50; /* ?????????????????? */
		
		humi = (SRH * 100.0) / 1024.0 / 1024;     /* ?????????????????? */
        temp = (ST * 200.0) / 1024.0 / 1024 - 50; /* ?????????????????? */

        return 0;
    }
    return -1;
}

static float aht21_get_humi(void)
{
	return humi;
}

static float aht21_get_temp(void)
{
	return temp;
}
