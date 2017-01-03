#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32f1xx_hal.h"
#include "tx_cfg.h"
#include "AP_OS.h"
#include <stdio.h>
extern osThreadId defaultTaskHandle;
extern osThreadId GPIOHandle;
extern osMessageQId SIG_GPIOHandle;
extern osMessageQId SIG_PLAYWAVHandle;

const uint32_t SIG_POWERKEY_DOWN = 0x0001;
const uint32_t SIG_POWERKEY_UP   = 0x0002;
const uint32_t SIG_USERKEY_DOWN  = 0x0004;
const uint32_t SIG_USERKEY_UP    = 0x0008;

__IO enum {SYS_close, SYS_ready, SYS_running} System_Status = SYS_close;


void Handle_System(void const * argument)
{
  uint32_t cnt;
  osEvent  evt;
  extern struct config SYS_CFG;
  printf(">>>System in ready mode\n");
  System_Status = SYS_ready;
  //System into READY
  while (osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_STARTUP, 0)) {
    ;
  }
  while(1)
  {
      evt = osMessageGet(SIG_GPIOHandle, 10);

      if (System_Status     == SYS_ready
      &&  evt.status        == osEventMessage
      &&  evt.value.signals &  SIG_POWERKEY_DOWN){
        cnt = 0;
        // wait for power key rise.
        while (1) {
          evt = osMessageGet(SIG_GPIOHandle, 1);

          if (evt.status        == osEventMessage 
          &&  evt.value.signals &  SIG_POWERKEY_UP)
          break;
          else cnt++;
        }
        printf("  Counting power key T:%dms\n", cnt);
        if (cnt >= (SYS_CFG.Tpoff>SYS_CFG.Tout ? SYS_CFG.Tout:SYS_CFG.Tpoff)) {
          if ((SYS_CFG.Tpoff >= SYS_CFG.Tout && SYS_CFG.Tpoff <= cnt)
          ||  (SYS_CFG.Tpoff <= SYS_CFG.Tout && SYS_CFG.Tout > cnt)) {
            printf(">>>System in close mode\n");  
            System_Status = SYS_close;
						//System into CLOSE
            while (osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_POWEROFF, 0)) {
              ;
            }
						osDelay(2000);
						HAL_GPIO_WritePin(Power_EN_GPIO_Port, Power_EN_Pin, GPIO_PIN_RESET);
						continue;
          }
          else {
            printf(">>>System in running mode\n");
            System_Status = SYS_running;
						//System into RUNNING
            while (osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_INTORUN, 0)) {
              ;
            }
						continue;
            
          }
        }
      } //end of System = ready, event == Power key

      if (System_Status     == SYS_running
      &&  evt.status        == osEventMessage
      &&  evt.value.signals &  SIG_POWERKEY_DOWN) {
        cnt = 0;
        // wait for power key rise.
        while (1) {
          evt = osMessageGet(SIG_GPIOHandle, 1);

          if (evt.status        == osEventMessage 
          &&  evt.value.signals &  SIG_POWERKEY_UP)
          break;
          else cnt++;
        }
        printf("  Counting power key T:%dms\n", cnt);
        if (cnt >= SYS_CFG.Tin) {
          printf(">>>System in ready mode\n");
          System_Status = SYS_ready;
					//System into READY
          while (osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_OUTRUN, 0)) {
            ;
          }
					continue;
          
        }
      } //end of System = running, event == Power key
  }
}

void Handle_GPIO(void const * argument)
{
  GPIO_PinState power;
  GPIO_PinState usr;
	GPIO_PinState GPIO_Buffer;
  power = HAL_GPIO_ReadPin(Power_KEY_GPIO_Port, Power_KEY_Pin);
  usr   = HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin);
  for(;;)
  {
		GPIO_Buffer = HAL_GPIO_ReadPin(Power_KEY_GPIO_Port, Power_KEY_Pin);
    if (power != GPIO_Buffer) {
      osDelay(10);
      GPIO_Buffer = HAL_GPIO_ReadPin(Power_KEY_GPIO_Port, Power_KEY_Pin);
      if (GPIO_Buffer == power)
      continue;

      power = GPIO_Buffer;
			if (GPIO_Buffer == GPIO_PIN_SET)
      printf("Power KEY down\n");
      else
			printf("Power KEY UP\n");
      if (GPIO_Buffer == GPIO_PIN_SET)
      while (osMessagePut(SIG_GPIOHandle, SIG_POWERKEY_DOWN, 0)) {
        ;
      }
      else
      while (osMessagePut(SIG_GPIOHandle, SIG_POWERKEY_UP, 0)) {
        ;
      }
			
			
    }
		GPIO_Buffer = HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin);
    if (usr != GPIO_Buffer) {
      osDelay(10);
      GPIO_Buffer = HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin);
      if (GPIO_Buffer == usr)
      continue;
      usr = GPIO_Buffer;
			if (GPIO_Buffer == GPIO_PIN_RESET)
      printf("User KEY down\n");
      else
			printf("User KEY UP\n");
      if (GPIO_Buffer == GPIO_PIN_RESET)
      while (osMessagePut(SIG_GPIOHandle, SIG_USERKEY_DOWN, 0)) {
        ;
      }
      else
      while (osMessagePut(SIG_GPIOHandle, SIG_USERKEY_UP, 0)) {
        ;
      }
    }
    osDelay(3);
  }
}
