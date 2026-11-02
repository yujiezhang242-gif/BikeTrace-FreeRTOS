/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bsp_uart_driver.h"
#include "bsp_gps_driver.h"
#include "iic.h"
#include "bsp_aht21_driver.h"
#include "bsp_aht21_handle.h"
#include "oled.h"
#include "bsp_timer_driver.h"
#include "bsp_key_driver.h"
#include "w25qxx.h"
#include "bsp_at24c02_driver.h"
#include "at_ec600n_tcp_driver.h"
#include "bsp_hmc5883l_driver.h"

#include "bike.h"
#include "screen.h"
#include "network.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  //MX_DMA_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  // Æô¶¯´®¿ÚÖÐ¶Ï½ÓÊÕ
  debug_uart_init();
  u2.start();
  u3.start();
  iic.init();
  aht21.init();
  
  tim.start();
  bike_init();
  hmc5883l.init();
  
//  printf("校准中，请旋转磁力计...\r\n");
  hmc5883l.calibration();
  
//  uint8_t readdata;
//  uint8_t writedata = 0xA5;
//  
//  at24c02.readBytes(&readdata, 1, 1);
//  printf("read data is [%x]\r\n", readdata);
//  at24c02.writeBytes(&writedata, 1, 1);
//  HAL_Delay(10);
//  at24c02.readBytes(&readdata, 1, 1);
//  printf("read data is [%x]\r\n", readdata);
  
//	printf("================================\r\n");
//	
//	HAL_Delay(200);
//	ee_Test();
//	HAL_Delay(200);
//	
//	printf("================================\r\n");
//uint8_t arrow=0;
//int16_t kx,ky;
//double lat = 22.23223, lng = 113.11113;
char* time_str = "123";
int32_t remain_m = 50;
float dis_km = 0;
//char orie[4];
//char inst[4];
//	at24c02.readBytes((uint8_t *)&kx, HMC_CALIB_KX_ADDR, 2);
//	at24c02.readBytes((uint8_t *)&ky, HMC_CALIB_KY_ADDR, 2);
////	at24c02.readBytes((uint8_t *)&lat, DEST_LAT_ADDR, 8);
////	at24c02.readBytes((uint8_t *)&lng, DEST_LNG_ADDR, 8);
//	  at24c02.writeBytes((uint8_t *)&lat, DEST_LAT_ADDR, 8);
//	  HAL_Delay(5);
//	  at24c02.writeBytes((uint8_t *)&lng, DEST_LNG_ADDR, 8);
//	at24c02.readBytes((uint8_t *)&lat, DEST_LAT_ADDR, 8);
//	at24c02.readBytes((uint8_t *)&lng, DEST_LNG_ADDR, 8);
//	char str1[12],str2[16];
//	sprintf(str1, "kx:%d,ky:%d",kx,ky);
//	sprintf(str2, "%.2f,%.2f",lat,lng);
  /* USER CODE END 2 */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
	  
	if (gps.handle.get_gprmc_flag())
	{
		gps.handle.parse_gprmc_data();
	}
	
	bike_handle();
	
	//if (check_ride_path_gener_done())
	{
		if (tim.getTimFlag())
		{
			//printf("keep living!\r\n");
			test(time_str, dis_km);
			dis_km += 0.01;
			tim.clearTimFlag();
			//LED_RIDE_TOGGLE();
		}
	}
	
	//HAL_Delay(1000);
	  
//	  OLED_ShowString(1,1,str1);
//	  OLED_ShowString(3,1,str2);
	
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
