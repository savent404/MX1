#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32f1xx_hal.h"
#include "tx_cfg.h"
#include "AP_OS.h"
#include <stdio.h>
#include "lis3D.h"
#include "tx_cfg.h"
#include "math.h"
#include "USR_CFG.h"
#include "DEBUG_CFG.h"
#include "LED.h"
#include "adc.h"

extern struct config SYS_CFG;
extern RGBL RGB_PROFILE[16][2];

extern osThreadId defaultTaskHandle;
extern osThreadId GPIOHandle;
extern osThreadId x3DList_CTLHandle;
extern osMessageQId SIG_GPIOHandle;
extern osMessageQId SIG_PLAYWAVHandle;
extern osMessageQId SIG_LEDHandle;
extern osTimerId TriggerFreezTimerHandle;

const uint32_t SIG_POWERKEY_DOWN = 0x0001;
const uint32_t SIG_POWERKEY_UP = 0x0002;
const uint32_t SIG_USERKEY_DOWN = 0x0004;
const uint32_t SIG_USERKEY_UP = 0x0008;


// AUDO off function
static uint32_t AUTO_READY_CNT = 0;
static uint32_t AUTO_OFF_CNT = 0;
static uint32_t AUTO_REMIND_FLAG = 0;

#define RESET_ALLTRIGGER_CNT() AUTO_READY_CNT=0,AUTO_OFF_CNT=0;

#define CHECK_READY_TRIGGER_CNT(x) if (SYS_CFG.Tautoin) {                      \
                                     if (System_Status == SYS_running &&\
                                         (AUTO_READY_CNT += 10) >= SYS_CFG.Tautoin) {       \
                                       AUTO_REMIND_FLAG = 1;                   \
                                     }                                         \
                                   }
#define CHECK_OFF_TRIGGER_CNT(x)   if (SYS_CFG.Tautooff) {                            \
                                     if (System_Status == SYS_ready && !CHARGE_FLAG &&        \
                                        (AUTO_OFF_CNT += 10) >= SYS_CFG.Tautooff) { \
                                       AUTO_REMIND_FLAG = 2;                          \
                                     }                                                \
                                   }


volatile static struct TFT {
  volatile int32_t TB;
  volatile int32_t TC;
  volatile int32_t TD;
} Trigger_Freeze_TIME;

/* System status in now */
__IO enum { SYS_close, SYS_ready, SYS_running } System_Status = SYS_close;

/* Bank now */
__IO uint8_t sBANK = 0;
/* Mute flag */
uint8_t MUTE_FLAG = 1;
/* Charging flag */
uint8_t CHARGE_FLAG = 0;
/* Charge complete flag */
uint8_t CC_FLAG = 0;

void Handle_System(void const* argument) {
  uint32_t cnt;
  osEvent evt;
  float power_voltag = 0;
  extern struct config SYS_CFG;
	//SD card Parameter error check
	
	// ADC check Power voltag
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 1);
  power_voltag = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 1);
  power_voltag = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 1);
  power_voltag = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 1);
  power_voltag = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
  power_voltag = power_voltag * SYS_VOLTAG / 4096.0;

  if (power_voltag <= LOWPOWER_VOLTAG) {
    printf_SYSTEM(">>>System put lowPower message\n");
    printf_SYSTEM("Power voltag:%.2fV\n", power_voltag);
    osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_LOWPOWER, osWaitForever);
		osDelay(2000);
  }
  
  printf_SYSTEM(">>>System in ready mode\n");
  if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET)
		MUTE_FLAG = 0;
  System_Status = SYS_ready;
  Trigger_Freeze_TIME.TB = 0, Trigger_Freeze_TIME.TC = 0,
  Trigger_Freeze_TIME.TD = 0;
  osTimerStart(TriggerFreezTimerHandle, 10);

  // System into READY
  if (MUTE_FLAG) {
    osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_STARTUP, PUT_MESSAGE_WAV_TIMEOUT);
		osDelay(2000);
  } RESET_ALLTRIGGER_CNT();

  while (1) {
    evt = osMessageGet(SIG_GPIOHandle, 10);
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, 1);
    power_voltag = HAL_ADC_GetValue(&hadc1);
		HAL_ADC_Stop(&hadc1);
    power_voltag = power_voltag * SYS_VOLTAG / 4096.0;
    if (power_voltag < RESTART_VOLTAG) {
      printf(">>>System restart :%.2fV\n", power_voltag);
			osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_OUTRUN_MUTE, osWaitForever);
      osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_RESTART, osWaitForever);
      osDelay(osWaitForever);
    }  // end of restart
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1, 1);
    power_voltag = HAL_ADC_GetValue(&hadc1);
		HAL_ADC_Stop(&hadc1);
    power_voltag = power_voltag * SYS_VOLTAG / 4096.0;

    // Charge check
    if (HAL_GPIO_ReadPin(Charge_Check_GPIO_Port, Charge_Check_Pin) == GPIO_PIN_SET && !CHARGE_FLAG) {
      osMessagePut(SIG_LEDHandle, SIG_LED_CHARGEA, PUT_MESSAGE_LED_TIMEOUT);
      if (System_Status == SYS_running) {
        osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_OUTRUN_MUTE, osWaitForever);
        osMessagePut(SIG_LEDHandle, SIG_LED_OUTRUN, osWaitForever);
				osMessagePut(SIG_LEDHandle, SIG_LED_CHARGEA, osWaitForever);
        System_Status = SYS_ready;
      }
      CHARGE_FLAG = 1;
      CC_FLAG = 0;
    } else if (HAL_GPIO_ReadPin(Charge_Check_GPIO_Port, Charge_Check_Pin) == GPIO_PIN_RESET && CHARGE_FLAG) {
      HAL_GPIO_WritePin(Power_EN_GPIO_Port, Power_EN_Pin, GPIO_PIN_RESET);
      CHARGE_FLAG = 0;
      CC_FLAG = 0;
      osMessagePut(SIG_LEDHandle, SIG_LED_OUTRUN, osWaitForever);
    }

    // Charge complete check
    if (CHARGE_FLAG && power_voltag >= CC_VOLTAG && !CC_FLAG) {
      osMessagePut(SIG_LEDHandle, SIG_LED_CHARGEB, osWaitForever);
      CC_FLAG = 1;

    } else if (CHARGE_FLAG && power_voltag < CC_VOLTAG && CC_FLAG) {
      CC_FLAG = 0;
      osMessagePut(SIG_LEDHandle, SIG_LED_CHARGEA, osWaitForever);
    }

    // If System in charge
    if (evt.status == osEventMessage && CHARGE_FLAG) {
      if (power_voltag < CC_VOLTAG) {
        if (System_Status == SYS_running) {
          osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_OUTRUN_MUTE,
                       PUT_MESSAGE_WAV_TIMEOUT);
          osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_CHARGE,
                       PUT_MESSAGE_WAV_TIMEOUT);
          System_Status = SYS_ready;
          osMessageGet(SIG_GPIOHandle, osWaitForever);
        } else {
          osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_CHARGE,
                       PUT_MESSAGE_WAV_TIMEOUT);
          osMessageGet(SIG_GPIOHandle, osWaitForever);
        }
      }
      continue;
    }

    if (AUTO_REMIND_FLAG == 1) {
      if (System_Status == SYS_running) {
        if (MUTE_FLAG)
          osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_OUTRUN, PUT_MESSAGE_WAV_TIMEOUT);
        osMessagePut(SIG_LEDHandle, SIG_LED_OUTRUN, PUT_MESSAGE_LED_TIMEOUT);
      }
      AUTO_REMIND_FLAG = 0;
      RESET_ALLTRIGGER_CNT();
			System_Status = SYS_ready;
    }
    else if (AUTO_REMIND_FLAG == 2) {
      if (System_Status == SYS_ready) {
        if (MUTE_FLAG)
          osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_POWEROFF, PUT_MESSAGE_WAV_TIMEOUT);
        osDelay(2000);
        HAL_GPIO_WritePin(Power_EN_GPIO_Port, Power_EN_Pin, GPIO_PIN_RESET);
        continue;
      }
      RESET_ALLTRIGGER_CNT();
      AUTO_REMIND_FLAG = 0;
    }


    if (System_Status == SYS_ready && evt.status == osEventMessage &&
        (evt.value.signals & SIG_POWERKEY_DOWN)) {
      cnt = 0;
      // wait for power key rise.
      while (1) {
        evt = osMessageGet(SIG_GPIOHandle, 1);

        if (evt.status == osEventMessage &&
        (evt.value.signals & SIG_POWERKEY_UP))
          break;
        else if (cnt++ > (SYS_CFG.Tpoff > SYS_CFG.Tout ? SYS_CFG.Tpoff : SYS_CFG.Tout))
          break;
      }
      printf_KEY("  Counting power key T:%dms\n", cnt);
      if (cnt >= (SYS_CFG.Tpoff > SYS_CFG.Tout ? SYS_CFG.Tout : SYS_CFG.Tpoff)) {
        if ((SYS_CFG.Tpoff >= SYS_CFG.Tout && SYS_CFG.Tpoff <= cnt) ||
            (SYS_CFG.Tpoff <= SYS_CFG.Tout && SYS_CFG.Tout > cnt)) {
          printf_SYSTEM(">>>System in close mode\n");
          System_Status = SYS_close;
          // System into CLOSE
          if (MUTE_FLAG)
            osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_POWEROFF,
                         PUT_MESSAGE_WAV_TIMEOUT);
          osDelay(2000);
          HAL_GPIO_WritePin(Power_EN_GPIO_Port, Power_EN_Pin, GPIO_PIN_RESET);
          continue;
        } else {
          printf_SYSTEM(">>>System in running mode\n");
          System_Status = SYS_running;
          // System into RUNNING
          if (MUTE_FLAG)
            osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_INTORUN,
                         PUT_MESSAGE_WAV_TIMEOUT);
          osMessagePut(SIG_LEDHandle, SIG_LED_INTORUN, PUT_MESSAGE_LED_TIMEOUT);
          RESET_ALLTRIGGER_CNT();
          continue;
        }
      }
    }  // end of System = ready, event == Power key

    if (System_Status == SYS_running && evt.status == osEventMessage &&
        evt.value.signals & SIG_POWERKEY_DOWN) {
      cnt = 0;
      // wait for power key rise.
      while (1) {
        evt = osMessageGet(SIG_GPIOHandle, 1);

        if (evt.status == osEventMessage && evt.value.signals & SIG_POWERKEY_UP)
          break;
        else if (evt.status == osEventMessage && evt.value.signals & SIG_USERKEY_DOWN)
        {
            printf_SYSTEM(">>>System put LED switch BANK\n");
            osMessagePut(SIG_LEDHandle, SIG_LED_SWITCHBANK,
                         PUT_MESSAGE_LED_TIMEOUT);
            osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_COLORSWITCH,
                         PUT_MESSAGE_WAV_TIMEOUT);
            RESET_ALLTRIGGER_CNT();
        }
        else if (cnt++ > SYS_CFG.Tin)
          break;
      }
      printf_KEY("  Counting power key T:%dms\n", cnt);
      if (cnt >= SYS_CFG.Tin) {
        printf_SYSTEM(">>>System in ready mode\n");
        System_Status = SYS_ready;
        // System into READY
        if (MUTE_FLAG)
          osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_OUTRUN,
                       PUT_MESSAGE_WAV_TIMEOUT);
        osMessagePut(SIG_LEDHandle, SIG_LED_OUTRUN, PUT_MESSAGE_LED_TIMEOUT);
        RESET_ALLTRIGGER_CNT();
        continue;
      }
    }  // end of System = running, event == Power key

    if (System_Status == SYS_ready && evt.status == osEventMessage &&
        evt.value.signals & SIG_USERKEY_DOWN) {
      cnt = 0;
      for (;;) {
        evt = osMessageGet(SIG_GPIOHandle, 1);

        if (evt.status == osEventMessage && evt.value.signals & SIG_USERKEY_UP)
          break;
        else if (cnt++ > SYS_CFG.Ts_switch)
          break;
      }
      printf_KEY("  Counting usr key T:%dms\n", cnt);
      if (cnt >= SYS_CFG.Ts_switch) {
        sBANK += 1;
        sBANK %= nBank;
        printf_SYSTEM(">>>System put bank switch message\n");
        if (MUTE_FLAG)
          osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_BANKSWITCH,
                       PUT_MESSAGE_WAV_TIMEOUT);
        RESET_ALLTRIGGER_CNT();
      }
      continue;
    }
    // end of System = ready, event == User Key

    if (System_Status == SYS_running && evt.status == osEventMessage &&
        evt.value.signals & SIG_USERKEY_DOWN) {
      cnt = 0;
      for (;;) {
        evt = osMessageGet(SIG_GPIOHandle, 1);

        if (evt.status == osEventMessage && evt.value.signals == SIG_USERKEY_UP)
          break;
        else if (evt.status == osEventMessage &&
                 evt.value.signals == SIG_POWERKEY_DOWN) {
          cnt = 0;
          break;
        } else if (++cnt >= SYS_CFG.TEtrigger) {
          printf_SYSTEM(">>>System put Trigger E\n");
          if (MUTE_FLAG)
            osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_TRIGGERE,
                         PUT_MESSAGE_WAV_TIMEOUT);
          osMessagePut(SIG_LEDHandle, SIG_LED_TRIGGERE,
                       PUT_MESSAGE_LED_TIMEOUT);
          RESET_ALLTRIGGER_CNT();
          break;
        }
      }
      if (cnt >= SYS_CFG.TEtrigger) {
        for (;;) {
          evt = osMessageGet(SIG_GPIOHandle, 1);
          if (evt.status == osEventMessage &&
              evt.value.signals & SIG_USERKEY_UP)
            break;
        }
        printf_SYSTEM(">>>System put Trigger E off\n");
        if (MUTE_FLAG)
          osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_TRIGGEREOFF,
                       PUT_MESSAGE_WAV_TIMEOUT);
        osMessagePut(SIG_LEDHandle, SIG_LED_TRIGGEREOFF,
                     PUT_MESSAGE_LED_TIMEOUT);
        RESET_ALLTRIGGER_CNT();
      } else if (!Trigger_Freeze_TIME.TD && cnt) {
        Trigger_Freeze_TIME.TD = SYS_CFG.TDfreeze;
        printf_SYSTEM(">>>System put Trigger D\n");
        if (MUTE_FLAG)
          osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_TRIGGERD,
                       PUT_MESSAGE_WAV_TIMEOUT);
        osMessagePut(SIG_LEDHandle, SIG_LED_TRIGGERD, PUT_MESSAGE_LED_TIMEOUT);
        RESET_ALLTRIGGER_CNT();
      } else {
//        printf_SYSTEM(">>>System put LED switch BANK\n");
//        osMessagePut(SIG_LEDHandle, SIG_LED_SWITCHBANK,
//                     PUT_MESSAGE_LED_TIMEOUT);
//        osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_COLORSWITCH,
//                     PUT_MESSAGE_WAV_TIMEOUT);
//        RESET_ALLTRIGGER_CNT();
      }
    }
    // end of System = running , event == User Key
  }
}

void Handle_GPIO(void const* argument) {
  GPIO_PinState power;
  GPIO_PinState usr;
  GPIO_PinState GPIO_Buffer;
  power = HAL_GPIO_ReadPin(Power_KEY_GPIO_Port, Power_KEY_Pin);
  usr = HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin);
  for (;;) {
    GPIO_Buffer = HAL_GPIO_ReadPin(Power_KEY_GPIO_Port, Power_KEY_Pin);
    if (power != GPIO_Buffer) {
      osDelay(10);
      GPIO_Buffer = HAL_GPIO_ReadPin(Power_KEY_GPIO_Port, Power_KEY_Pin);
      if (GPIO_Buffer == power) continue;

      power = GPIO_Buffer;
      if (GPIO_Buffer == GPIO_PIN_SET) {
        printf_KEY("Power KEY down\n");
      } else {
        printf_KEY("Power KEY UP\n");
      }
      if (GPIO_Buffer == GPIO_PIN_SET)
        osMessagePut(SIG_GPIOHandle, SIG_POWERKEY_DOWN, osWaitForever);
      else
        osMessagePut(SIG_GPIOHandle, SIG_POWERKEY_UP, osWaitForever);
    }
    GPIO_Buffer = HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin);
    if (usr != GPIO_Buffer) {
      osDelay(10);
      GPIO_Buffer = HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin);
      if (GPIO_Buffer == usr) continue;
      usr = GPIO_Buffer;
      if (GPIO_Buffer == GPIO_PIN_RESET) {
        printf_KEY("User KEY down\n");
      } else {
        printf_KEY("User KEY UP\n");
      }
      if (GPIO_Buffer == GPIO_PIN_RESET)
        osMessagePut(SIG_GPIOHandle, SIG_USERKEY_DOWN, osWaitForever);
      else
        osMessagePut(SIG_GPIOHandle, SIG_USERKEY_UP, osWaitForever);
    }
    osDelay(3);
  }
}



float log_ans;

void x3DListHandle(void const* argument) {
  while (1){
    uint8_t move,click, __move, __click;
    static  uint8_t _move = 0, _click = 0;

    __move = Lis3d_isMove();
    __click = Lis3d_isClick();

    if (__move > 0 && _move == 0) move = 1;
    else move = 0;
    if (__click > 0 && _click == 0) click = 1;
    else click = 0;
    _move = __move;
    _click = __click;

    osDelay(20);
    if (System_Status != SYS_running) {
      continue;
    }
    
    if (click && !Trigger_Freeze_TIME.TC) {
      Trigger_Freeze_TIME.TC = SYS_CFG.TCfreeze;
      if (MUTE_FLAG) osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_TRIGGERC, 10);
      osMessagePut(SIG_LEDHandle, SIG_LED_TRIGGERC, 10);
      RESET_ALLTRIGGER_CNT();
      continue;
    }
    else if (move && !Trigger_Freeze_TIME.TB) {
      Trigger_Freeze_TIME.TB = SYS_CFG.TBfreeze;
      if (MUTE_FLAG) osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_TRIGGERB, 10);
      osMessagePut(SIG_LEDHandle, SIG_LED_TRIGGERB, 10);
      RESET_ALLTRIGGER_CNT();
      continue;
    }
  }
}

void TriggerFreez(void const* argument) {
	static int cnt = 0;
	extern Simple_LED_T SLT[nBank];

	if (++cnt > SLT[sBANK] / 10) {
		Simple_LED_Opt();
		cnt = 0;
	}

  CHECK_READY_TRIGGER_CNT();
	CHECK_OFF_TRIGGER_CNT();

  if (Trigger_Freeze_TIME.TB > 10)
    Trigger_Freeze_TIME.TB -= 10;
  else if (Trigger_Freeze_TIME.TB > 0)

  {
    printf_TRIGGERFREEZ("Trigger B reset\n");
    Trigger_Freeze_TIME.TB = 0;
  } else if (Trigger_Freeze_TIME.TB < 0) {
    printf_TRIGGERFREEZ("Trigger B reset\n");
    Trigger_Freeze_TIME.TB = 0;
  }

  if (Trigger_Freeze_TIME.TC > 10)
    Trigger_Freeze_TIME.TC -= 10;
  else if (Trigger_Freeze_TIME.TC > 0) {
    printf_TRIGGERFREEZ("Trigger C reset\n");
    Trigger_Freeze_TIME.TC = 0;
  } else if (Trigger_Freeze_TIME.TC < 0) {
    printf_TRIGGERFREEZ("Trigger C reset\n");
    Trigger_Freeze_TIME.TC = 0;
  }

  if (Trigger_Freeze_TIME.TD > 10)
    Trigger_Freeze_TIME.TD -= 10;
  else if (Trigger_Freeze_TIME.TD > 0) {
    printf_TRIGGERFREEZ("Trigger D reset\n");
    Trigger_Freeze_TIME.TD = 0;
  } else if (Trigger_Freeze_TIME.TD < 0) {
    printf_TRIGGERFREEZ("Trigger D reset\n");
    Trigger_Freeze_TIME.TD = 0;
  }
}
