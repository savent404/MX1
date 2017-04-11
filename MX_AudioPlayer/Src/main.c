/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "fatfs.h"
#include "sdio.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include "LED.h"
#include "Lis3D.h"
#include "AF.h"
#include "AP.h"
#include "string.h"
#include "tx_cfg.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
void MX_FREERTOS_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
FATFS fs;
FRESULT res;
struct config SYS_CFG;

/* LED public var */
Simple_LED SL[nBank][256];
Simple_LED_T SLT[nBank];
Simple_LED_STEP SLS[nBank];
RGBL RGB_PROFILE[16][2];

uint32_t i;

/* Lis3dh */
Lis3dConfig lis3d;

/**
 * @brief: check SETTING file's parameter
 * @Retvl: 0-OK / 1-Error
 * @Note : some parameter will be changed without Error return
 */
static uint8_t parameter_check(struct config p) {
  // Vol 
  if (SYS_CFG.Vol > 3) {
    SYS_CFG.Vol = 3;
  }

  // Tpon
  if (SYS_CFG.Tpon < 800) {
    SYS_CFG.Tpon = 800;
  }

  // Tin & Ts_switch
  if (SYS_CFG.Tin >= SYS_CFG.TEtrigger ||
      SYS_CFG.Ts_switch >= SYS_CFG.TEtrigger)
      return 1;
  
  if (SYS_CFG.TLcolor > SYS_CFG.TEtrigger ||
      SYS_CFG.TLcolor > SYS_CFG.Tin)
      return 1;
  
  if (SYS_CFG.Ldeep > SYS_CFG.Lbright)
    SYS_CFG.Ldeep = 0;
  
//  if (SYS_CFG.Sl > SYS_CFG.Sh ||
//      SYS_CFG.Cl > SYS_CFG.Ch)
//      return 1;
//  if (SYS_CFG.Sl > 1023||
//      SYS_CFG.Sh > 1023||
//      SYS_CFG.Cl > 1023||
//      SYS_CFG.Ch > 1023)
//      return 1;
  return 0;
}
/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */
	extern uint16_t static_wav[22001];
  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_DAC_Init();
  MX_SDIO_SD_Init();
  MX_SPI2_Init();
  MX_TIM2_Init();
  MX_TIM1_Init();

  /* USER CODE BEGIN 2 */
  Lis3d_Init();
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
  MX_FATFS_Init();
  res = f_mount(&fs, (const TCHAR*)"0", 1);

	// SD card check
	if (res != FR_OK)
	{
		HAL_GPIO_WritePin(Power_EN_GPIO_Port, Power_EN_Pin, GPIO_PIN_SET);
		HAL_TIM_Base_Start(&htim2);
		HAL_GPIO_WritePin(Audio_EN_GPIO_Port, Audio_EN_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Audio_Soft_EN_GPIO_Port, Audio_Soft_EN_Pin,
		GPIO_PIN_SET);
		HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)static_wav,
		22001, DAC_ALIGN_12B_R);
		HAL_Delay(1000);
		while (HAL_GPIO_ReadPin(Power_KEY_GPIO_Port, Power_KEY_Pin)){
		;
		}
		HAL_GPIO_WritePin(Power_EN_GPIO_Port, Power_EN_Pin, GPIO_PIN_RESET);
		HAL_Delay(5000);
	}
	TX_CFG(&SYS_CFG,RGB_PROFILE);
	
	// System Poweron
	HAL_Delay(SYS_CFG.Tpon);
  HAL_GPIO_WritePin(Power_EN_GPIO_Port, Power_EN_Pin, GPIO_PIN_SET);
	
	// Read Accent structure;
	{
		uint8_t Bank_num = 0;
		FIL     file;
		char    path[20];
		char    para_buf[20];
		char*   spt;
		for (Bank_num = 0; Bank_num < nBank; Bank_num++) {
			sprintf(path, "0:/Bank%d/Accent.txt", Bank_num + 1);
			res = f_open(&file, path, FA_READ);
			if (res != FR_OK) break;
			//Read Parameter
			{
				uint8_t cnt = 0;
				spt = f_gets(para_buf, sizeof(para_buf), &file);
				if (spt==0) {res = FR_INVALID_OBJECT; break;}
				sscanf(spt, "T=%d", &SLT[Bank_num]);
				while (1) {
					
					uint32_t para;
					f_gets(para_buf, sizeof(para_buf), &file);
					if (spt==0 || *spt==0){
						SLS[Bank_num] = cnt + 1;
						break;
					}
					sscanf(spt, "%ud", &para);
          SL[Bank_num][cnt].LED8 = para%10; para /= 10;
          SL[Bank_num][cnt].LED7 = para%10; para /= 10;
          SL[Bank_num][cnt].LED6 = para%10; para /= 10;
          SL[Bank_num][cnt].LED5 = para%10; para /= 10;
          SL[Bank_num][cnt].LED4 = para%10; para /= 10;
          SL[Bank_num][cnt].LED3 = para%10; para /= 10;
          SL[Bank_num][cnt].LED2 = para%10; para /= 10;
          SL[Bank_num][cnt].LED1 = para%10; para /= 10;

					cnt++;
				}
			}
			f_close(&file);
		}
		
	}
	
  lis3d.MD = SYS_CFG.MD;
  lis3d.MT = SYS_CFG.MT;
  lis3d.CD = SYS_CFG.CD;
  lis3d.CT = SYS_CFG.CT;
  lis3d.CL = SYS_CFG.CL;
  lis3d.CW = SYS_CFG.CW;
	Lis3d_Set((Lis3dConfig*)&lis3d);
	// Parameter check
	if (parameter_check(SYS_CFG) || res != FR_OK)
	{
		HAL_GPIO_WritePin(Power_EN_GPIO_Port, Power_EN_Pin, GPIO_PIN_SET);
		HAL_TIM_Base_Start(&htim2);
		HAL_GPIO_WritePin(Audio_EN_GPIO_Port, Audio_EN_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(Audio_Soft_EN_GPIO_Port, Audio_Soft_EN_Pin,
		GPIO_PIN_SET);
		HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)static_wav,
		2001, DAC_ALIGN_12B_R);
		HAL_Delay(200);
		HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
		HAL_Delay(200);
		HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)static_wav,
		2001, DAC_ALIGN_12B_R);
		HAL_Delay(200);
		while (HAL_GPIO_ReadPin(Power_KEY_GPIO_Port, Power_KEY_Pin)){
		;
		}
		HAL_GPIO_WritePin(Power_EN_GPIO_Port, Power_EN_Pin, GPIO_PIN_RESET);
		HAL_Delay(5000);
	}
  
	// while (HAL_GPIO_ReadPin(Power_KEY_GPIO_Port, Power_KEY_Pin)){
	// 	;
	// }
	
	
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

    /**Initializes the CPU, AHB and APB busses clocks 
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

  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM8 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
/* USER CODE BEGIN Callback 0 */

/* USER CODE END Callback 0 */
  if (htim->Instance == TIM8) {
    HAL_IncTick();
  }
/* USER CODE BEGIN Callback 1 */

/* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler */
  /* User can add his own implementation to report the HAL error return state */
  while(1) 
  {
  }
  /* USER CODE END Error_Handler */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
