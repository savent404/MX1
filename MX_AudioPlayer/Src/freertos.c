/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  *
  * COPYRIGHT(c) 2017 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/* USER CODE BEGIN Includes */     
#include "stdio.h"
/* USER CODE END Includes */

/* Variables -----------------------------------------------------------------*/
osThreadId defaultTaskHandle;
osThreadId GPIOHandle;
osThreadId WAV_CTLHandle;
osThreadId DAC_CTLHandle;
osThreadId x3DList_CTLHandle;
osThreadId LED_CTLHandle;
osMessageQId pWAVHandle;
osMessageQId SIG_GPIOHandle;
osMessageQId SIG_PLAYWAVHandle;
osMessageQId SIG_LEDHandle;
osTimerId TriggerFreezTimerHandle;
osSemaphoreId DMA_FLAGHandle;

/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

/* Function prototypes -------------------------------------------------------*/
void Handle_System(void const * argument);
void Handle_GPIO(void const * argument);
void WAVHandle(void const * argument);
void DACHandle(void const * argument);
void x3DListHandle(void const * argument);
void LEDHandle(void const * argument);
void TriggerFreez(void const * argument);

extern void MX_FATFS_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 4 */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
	printf("Handle OVERFLOW:%s\n", pcTaskName);
}
/* USER CODE END 4 */

/* Init FreeRTOS */

void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
       
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of DMA_FLAG */
  osSemaphoreDef(DMA_FLAG);
  DMA_FLAGHandle = osSemaphoreCreate(osSemaphore(DMA_FLAG), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* definition and creation of TriggerFreezTimer */
  osTimerDef(TriggerFreezTimer, TriggerFreez);
  TriggerFreezTimerHandle = osTimerCreate(osTimer(TriggerFreezTimer), osTimerPeriodic, NULL);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, Handle_System, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of GPIO */
  osThreadDef(GPIO, Handle_GPIO, osPriorityIdle, 0, 128);
  GPIOHandle = osThreadCreate(osThread(GPIO), NULL);

  /* definition and creation of WAV_CTL */
  osThreadDef(WAV_CTL, WAVHandle, osPriorityRealtime, 0, 350);
  WAV_CTLHandle = osThreadCreate(osThread(WAV_CTL), NULL);

  /* definition and creation of DAC_CTL */
  osThreadDef(DAC_CTL, DACHandle, osPriorityHigh, 0, 128);
  DAC_CTLHandle = osThreadCreate(osThread(DAC_CTL), NULL);

  /* definition and creation of x3DList_CTL */
  osThreadDef(x3DList_CTL, x3DListHandle, osPriorityNormal, 0, 250);
  x3DList_CTLHandle = osThreadCreate(osThread(x3DList_CTL), NULL);

  /* definition and creation of LED_CTL */
  osThreadDef(LED_CTL, LEDHandle, osPriorityLow, 0, 128);
  LED_CTLHandle = osThreadCreate(osThread(LED_CTL), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the queue(s) */
  /* definition and creation of pWAV */
  osMessageQDef(pWAV, 3, uint32_t);
  pWAVHandle = osMessageCreate(osMessageQ(pWAV), NULL);

  /* definition and creation of SIG_GPIO */
  osMessageQDef(SIG_GPIO, 4, uint8_t);
  SIG_GPIOHandle = osMessageCreate(osMessageQ(SIG_GPIO), NULL);

  /* definition and creation of SIG_PLAYWAV */
  osMessageQDef(SIG_PLAYWAV, 4, uint8_t);
  SIG_PLAYWAVHandle = osMessageCreate(osMessageQ(SIG_PLAYWAV), NULL);

  /* definition and creation of SIG_LED */
  osMessageQDef(SIG_LED, 4, uint32_t);
  SIG_LEDHandle = osMessageCreate(osMessageQ(SIG_LED), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
}

/* Handle_System function */
__weak void Handle_System(void const * argument)
{
  /* init code for FATFS */
  MX_FATFS_Init();

  /* USER CODE BEGIN Handle_System */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Handle_System */
}

/* Handle_GPIO function */
__weak void Handle_GPIO(void const * argument)
{
  /* USER CODE BEGIN Handle_GPIO */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Handle_GPIO */
}

/* WAVHandle function */
__weak void WAVHandle(void const * argument)
{
  /* USER CODE BEGIN WAVHandle */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END WAVHandle */
}

/* DACHandle function */
__weak void DACHandle(void const * argument)
{
  /* USER CODE BEGIN DACHandle */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END DACHandle */
}

/* x3DListHandle function */
__weak void x3DListHandle(void const * argument)
{
  /* USER CODE BEGIN x3DListHandle */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END x3DListHandle */
}

/* LEDHandle function */
__weak void LEDHandle(void const * argument)
{
  /* USER CODE BEGIN LEDHandle */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END LEDHandle */
}

/* TriggerFreez function */
__weak void TriggerFreez(void const * argument)
{
  /* USER CODE BEGIN TriggerFreez */
  
  /* USER CODE END TriggerFreez */
}

/* USER CODE BEGIN Application */
     
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
