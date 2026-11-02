/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

#define WEIGHT_KG  														 (70)
#define WEIGHT_FACTOR													(0.5)

/* ========================= 断电续骑部分参数保存 ========================= */
#define RIDE_DONE_FLAG                                                 (0xA5)
#define ERASE_FLAG                                                     (0XFF)
#define PAGE_INDEX_ADDR                            		        		(0) //1
#define WRITE_FLAG_ADDR                          		        		(1) //1
#define RIDE_RECORE_COUNT_ADDR                                 			(2) //2
#define PAGE_SIZE                                                      (256)

/* ========================= 磁力计校准参数保存 ========================= */
#define HMC_CALIB_KX_ADDR                                               (4) //2
#define HMC_CALIB_KY_ADDR                                               (6) //2
/* ========================= 目的地经纬度参数保存 ========================= */
#define DEST_LAT_ADDR                                                   (8) //8 维度
#define DEST_LNG_ADDR                                                  (16) //8 经度

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KEY1_Pin GPIO_PIN_14
#define KEY1_GPIO_Port GPIOC
#define KEY2_Pin GPIO_PIN_0
#define KEY2_GPIO_Port GPIOB
#define IIC_SCL_Pin GPIO_PIN_6
#define IIC_SCL_GPIO_Port GPIOB
#define IIC_SDA_Pin GPIO_PIN_7
#define IIC_SDA_GPIO_Port GPIOB
#define W25QXX_CS_Pin GPIO_PIN_4
#define W25QXX_CS_GPIO_Port GPIOA
#define LED_GPS_Pin GPIO_PIN_1
#define LED_GPS_GPIO_Port GPIOA
#define LED_RIDE_Pin GPIO_PIN_0
#define LED_RIDE_GPIO_Port GPIOA
#define LED_Port GPIOA

#define LED_GPS_ON()     HAL_GPIO_WritePin(LED_Port, LED_GPS_Pin, GPIO_PIN_RESET)
#define LED_GPS_OFF()    HAL_GPIO_WritePin(LED_Port, LED_GPS_Pin, GPIO_PIN_SET)
#define LED_GPS_TOGGLE() HAL_GPIO_TogglePin(LED_Port, LED_GPS_Pin)

#define LED_RIDE_ON()    HAL_GPIO_WritePin(LED_Port, LED_RIDE_Pin, GPIO_PIN_RESET)
#define LED_RIDE_OFF()   HAL_GPIO_WritePin(LED_Port, LED_RIDE_Pin, GPIO_PIN_SET)
#define LED_RIDE_TOGGLE() HAL_GPIO_TogglePin(LED_Port, LED_RIDE_Pin)
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
