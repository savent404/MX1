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
uint8_t MUTE_FLAG = 0;
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
  power_voltag = power_voltag * 3.3 / 4096.0;

  if (power_voltag <= LOWPOWER_VOLTAG && power_voltag >= RESTART_VOLTAG) {
    printf_SYSTEM(">>>System put lowPower message\n");
    printf_SYSTEM("Power voltag:%.2fV\n", power_voltag);
    osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_LOWPOWER, osWaitForever);
  }
  printf_SYSTEM(">>>System in ready mode\n");
  if (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_SET) MUTE_FLAG = 1;
  System_Status = SYS_ready;
  Trigger_Freeze_TIME.TB = 0, Trigger_Freeze_TIME.TC = 0,
  Trigger_Freeze_TIME.TD = 0;
  osTimerStart(TriggerFreezTimerHandle, 10);

  // System into READY
  if (MUTE_FLAG)
    osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_STARTUP, PUT_MESSAGE_WAV_TIMEOUT);
  while (1) {
    evt = osMessageGet(SIG_GPIOHandle, 10);
    if (power_voltag < 1.70) {
      printf(">>>System restart :%.2fV\n", power_voltag);
      osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_RESTART, osWaitForever);
      osDelay(osWaitForever);
    }  // end of restart
    power_voltag = HAL_ADC_GetValue(&hadc1);
    power_voltag = power_voltag * 3.3 / 4096.0;

    // Charge check
    if (HAL_GPIO_ReadPin(Charge_Check_GPIO_Port, Charge_Check_Pin) ==
            GPIO_PIN_SET &&
        !CHARGE_FLAG) {
      osMessagePut(SIG_LEDHandle, SIG_LED_CHARGEA, PUT_MESSAGE_LED_TIMEOUT);
      if (System_Status == SYS_running) {
        osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_OUTRUN_MUTE, osWaitForever);
        osMessagePut(SIG_LEDHandle, SIG_LED_OUTRUN, osWaitForever);
        System_Status = SYS_ready;
      }
      CHARGE_FLAG = 1;
      CC_FLAG = 0;
    } else if (HAL_GPIO_ReadPin(Charge_Check_GPIO_Port, Charge_Check_Pin) ==
                   GPIO_PIN_RESET &&
               CHARGE_FLAG) {
      CHARGE_FLAG = 0;
      CC_FLAG = 0;
      osMessagePut(SIG_LEDHandle, SIG_LED_OUTRUN, osWaitForever);
    }

    // Charge complete check
    if (CHARGE_FLAG && power_voltag > CC_VOLTAG && !CC_FLAG) {
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
    if (System_Status == SYS_ready && evt.status == osEventMessage &&
        evt.value.signals & SIG_POWERKEY_DOWN) {
      cnt = 0;
      // wait for power key rise.
      while (1) {
        evt = osMessageGet(SIG_GPIOHandle, 1);

        if (evt.status == osEventMessage && evt.value.signals & SIG_POWERKEY_UP)
          break;
        else
          cnt++;
      }
      printf_KEY("  Counting power key T:%dms\n", cnt);
      if (cnt >=
          (SYS_CFG.Tpoff > SYS_CFG.Tout ? SYS_CFG.Tout : SYS_CFG.Tpoff)) {
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
        else
          cnt++;
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
        else
          cnt++;
      }
      printf_KEY("  Counting usr key T:%dms\n", cnt);
      if (cnt >= SYS_CFG.Ts_switch) {
        sBANK += 1;
        sBANK %= nBank;
        printf_SYSTEM(">>>System put bank switch message\n");
        if (MUTE_FLAG)
          osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_BANKSWITCH,
                       PUT_MESSAGE_WAV_TIMEOUT);
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
      } else if (!Trigger_Freeze_TIME.TD && cnt) {
        Trigger_Freeze_TIME.TD = SYS_CFG.TDfreeze;
        printf_SYSTEM(">>>System put Trigger D\n");
        if (MUTE_FLAG)
          osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_TRIGGERD,
                       PUT_MESSAGE_WAV_TIMEOUT);
        osMessagePut(SIG_LEDHandle, SIG_LED_TRIGGERD, PUT_MESSAGE_LED_TIMEOUT);
      } else {
        printf_SYSTEM(">>>System put LED switch BANK\n");
        osMessagePut(SIG_LEDHandle, SIG_LED_SWITCHBANK,
                     PUT_MESSAGE_LED_TIMEOUT);
        osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_COLORSWITCH,
                     PUT_MESSAGE_WAV_TIMEOUT);
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
void x3DListHandle(void const* argument) {
  Lis3dData data;
  float ans;
  taskENTER_CRITICAL();
  Lis3d_Init();
  taskEXIT_CRITICAL();
  for (;;) {
    if (System_Status != SYS_running) {
      osDelay(50);
      continue;
    }

    taskENTER_CRITICAL();
    Lis3dGetData(&data);
    taskEXIT_CRITICAL();
		ans = 0;
    ans = data.Dx * data.Dx;
    ans += data.Dz * data.Dz;
    ans = sqrt(ans);
		// +/- 4g / 0x10000 = +/- 0.5
		// +/- 0.5 * 1024 * 2 = +/- 1024
    ans = ans * 1024 * 2/ 0x10000;
		{
			const int BL = 85;
			const float B[85] = {
				 -0.05936408415,-0.004936652724,-0.005133451428,-0.005327147432,-0.005524651147,
				-0.005719020031,-0.005905815866,-0.006116441451,-0.006294964813,-0.006483847741,
				 -0.00667139329, -0.00686581945, -0.00705115404,-0.007231596857,-0.007410131395,
				-0.007585029583,-0.007757484913,-0.007930437103,-0.008101559244,-0.008268840611,
				-0.008425734937,-0.008575976826,-0.008717850782,-0.008855309337,-0.008985036053,
				-0.009110610001,-0.009228942916,-0.009343555197,-0.009447877295,-0.009551500902,
				-0.009652191773,-0.009749913588,-0.009835532866,-0.009904609993,-0.009972687811,
				 -0.01001082174, -0.01005535573, -0.01008627471, -0.01017241273, -0.01020499412,
				 -0.01016818453, -0.01022991724,   0.9897991419, -0.01022991724, -0.01016818453,
				 -0.01020499412, -0.01017241273, -0.01008627471, -0.01005535573, -0.01001082174,
				-0.009972687811,-0.009904609993,-0.009835532866,-0.009749913588,-0.009652191773,
				-0.009551500902,-0.009447877295,-0.009343555197,-0.009228942916,-0.009110610001,
				-0.008985036053,-0.008855309337,-0.008717850782,-0.008575976826,-0.008425734937,
				-0.008268840611,-0.008101559244,-0.007930437103,-0.007757484913,-0.007585029583,
				-0.007410131395,-0.007231596857, -0.00705115404, -0.00686581945, -0.00667139329,
				-0.006483847741,-0.006294964813,-0.006116441451,-0.005905815866,-0.005719020031,
				-0.005524651147,-0.005327147432,-0.005133451428,-0.004936652724, -0.05936408415
			};
			static uint8_t pos = 0;
			static float shift[85] = {0};
			uint8_t i;
			shift[pos] = ans;
			ans = 0;
			for (i = 0; i < BL; i++)
				ans += B[i] * shift[(BL + pos - i)%BL];
		}
		ans = ans > 0? ans : -ans;
		ans *= 10;
    // Trigger B
    if (SYS_CFG.Cl <= ans && SYS_CFG.Ch >= ans && !Trigger_Freeze_TIME.TC) {
      Trigger_Freeze_TIME.TC = SYS_CFG.TCfreeze;
      printf_SYSTEM(">>>System put Trigger C\n");
      printf_SYSTEM("<<<3DH:%.2f\n", ans);
      if (MUTE_FLAG) osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_TRIGGERC, 10);
      osMessagePut(SIG_LEDHandle, SIG_LED_TRIGGERC, 10);
      continue;
    }
    // Trigger C
    else if (SYS_CFG.S1 <= ans && SYS_CFG.Sh >= ans &&
             !Trigger_Freeze_TIME.TB) {
      Trigger_Freeze_TIME.TB = SYS_CFG.TBfreeze;
      printf_SYSTEM(">>>System put Trigger B\n");
      printf_SYSTEM("<<<3DH:%.2f\n", ans);
      if (MUTE_FLAG) osMessagePut(SIG_PLAYWAVHandle, SIG_AUDIO_TRIGGERB, 10);
      osMessagePut(SIG_LEDHandle, SIG_LED_TRIGGERB, 10);
      continue;
    }

    osDelay(20);
  }
}

void TriggerFreez(void const* argument) {
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
