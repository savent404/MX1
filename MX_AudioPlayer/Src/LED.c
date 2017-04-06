#include "LED.h"
#include "DEBUG_CFG.h"
#include "USR_CFG.h"
#include "tx_cfg.h"
extern struct config SYS_CFG;
extern uint8_t sBANK;
extern osMessageQId SIG_LEDHandle;
extern RGBL RGB_PROFILE[16][2];
/* When System from Close into Ready */
const uint8_t SIG_LED_STARTUP = 1;

/* When System from Ready into Close */
const uint8_t SIG_LED_POWEROFF = 2;

/* When SYstem from Ready into running */
const uint8_t SIG_LED_INTORUN = 3;

/* When System from Running into Reayd */
const uint8_t SIG_LED_OUTRUN = 4;

/* When System in Ready switch the Bank */
const uint8_t SIG_LED_BANKSWITCH = 5;

/* Triggers */
/* When System in Running, Trigger B */
const uint8_t SIG_LED_TRIGGERB = 0x10;
/* When System in Running, Trigger C */
const uint8_t SIG_LED_TRIGGERC = 0x20;
/* When System in Running, Trigger D */
const uint8_t SIG_LED_TRIGGERD = 0x30;
/* When System in Running, Trigger E */
const uint8_t SIG_LED_TRIGGERE = 0x40;
/* When System in Running, Trigger E off */
const uint8_t SIG_LED_TRIGGEREOFF = 0x50;
/* When charge power .ing */
const uint8_t SIG_LED_CHARGEA = 0xFF;
/* WHen charge power .done */
const uint8_t SIG_LED_CHARGEB = 0xEE;

const uint8_t SIG_LED_SWITCHBANK = 0x60;

static uint8_t shift_bank = 0;
static int multi_trigger_func(uint32_t mode);


void LEDHandle(void const *argument) {
  osEvent evt;
  
  enum { IN_TRIGGER_E, OUT_TRIGGER_E } trigger_e = OUT_TRIGGER_E;
	uint8_t Normal_Mode_STACK = 0;
	uint8_t Normal_Mode_STACK_ENABLE = 0;
  uint8_t flag = 1;
  uint8_t jump_flag = 0;
  printf_LED("LED Handle init start\n");
  HAL_TIM_Base_Start(&htim1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
  while (1) {
		if (Normal_Mode_STACK_ENABLE & Normal_Mode_STACK && trigger_e == OUT_TRIGGER_E) {
      jump_flag = 1;
			evt.value.v = SIG_LED_INTORUN;
			goto TAG;
		}
    if (trigger_e == OUT_TRIGGER_E) {
      evt = osMessageGet(SIG_LEDHandle, osWaitForever);
    } else {
      evt = osMessageGet(SIG_LEDHandle, T_Electricl / 2 /*SYS_CFG.Ecycle / 2*/);
    }
    if (trigger_e == IN_TRIGGER_E) {
      LED_COLOR_SET(RGB_PROFILE[(sBANK + shift_bank) % nBank][flag], 0xFF, 1);
      printf_LED("Trigger E Half Cycle hit\n");
      if (flag)
        flag = 0;
      else
        flag = 1;
    }
    if (evt.status != osEventMessage) continue;
		

    printf_LED("LED message GET\n");
TAG:
    switch (evt.value.v) {
      case SIG_LED_INTORUN: {
        
        printf_LED("&LED\tGet mode running message\n");
				
				Normal_Mode_STACK_ENABLE = 1;
				Normal_Mode_STACK = 1;
        
        switch (SYS_CFG.LMode) {
          // STATIC
          case 1: {
						uint16_t cnt;

            if (!jump_flag) {
              for (cnt = 1; cnt < SYS_CFG.TLon / 10; cnt++) {
                LED_COLOR_SET(RGB_PROFILE[(sBANK + shift_bank) % nBank][0],
                              0xFF * cnt / (SYS_CFG.TLon / 10), 1);
                osDelay(10);
              }
            }
            jump_flag = 0;
            LED_COLOR_SET(RGB_PROFILE[(sBANK + shift_bank) % nBank][0], 0xFF, 1);
            evt = osMessageGet(SIG_LEDHandle, osWaitForever);
            if (evt.status == osEventMessage) goto TAG;
          } break;
          // Breath
          case 2: {
            uint16_t i;
            while (1) {
              for (i = T_Breath - 1; i > 0; i--) {
                LED_COLOR_SET(
                    RGB_PROFILE[(sBANK + shift_bank) % nBank][0],
                    i * (0xFF - (SYS_CFG.Lbright - SYS_CFG.Ldeep)) / T_Breath + (SYS_CFG.Lbright - SYS_CFG.Ldeep), 1);
                evt = osMessageGet(SIG_LEDHandle, 1);
                if (evt.status == osEventMessage) goto TAG;
              }
              for (i = 1; i < T_Breath - 1; i++) {
                LED_COLOR_SET(
                    RGB_PROFILE[(sBANK + shift_bank) % nBank][0],
                    i * (0xFF - (SYS_CFG.Lbright - SYS_CFG.Ldeep)) / T_Breath + (SYS_CFG.Lbright - SYS_CFG.Ldeep), 1);
                evt = osMessageGet(SIG_LEDHandle, 1);
                if (evt.status == osEventMessage) goto TAG;
              }
            }
          }
          // SLOW PLUSE & Mid PLUSE & FAST PLUSE
          case 3:
          case 4:
          case 5: {
            const uint16_t delay_time[] = {0, 0, 0, T_SP, T_MP, T_FP};
            while (1) {
              srand(SysTick->VAL);
              LED_COLOR_SET(RGB_PROFILE[(sBANK + shift_bank) % nBank][0],
                            rand() % (0xFF - (SYS_CFG.Lbright - SYS_CFG.Ldeep)) + (SYS_CFG.Lbright - SYS_CFG.Ldeep), 1);
              evt = osMessageGet(SIG_LEDHandle, delay_time[SYS_CFG.LMode]);
              if (evt.status == osEventMessage) goto TAG;
            }
          }
        }
      }
      case SIG_LED_OUTRUN: {
        uint16_t cnt;
        printf_LED("&LED\tGet mode ready message\n");
				Normal_Mode_STACK = 0;
				Normal_Mode_STACK_ENABLE = 0;
        cnt = SYS_CFG.TLoff / 10;
        while (cnt--) {
          LED_COLOR_SET(RGB_PROFILE[0][0], 0xFF * cnt / (SYS_CFG.TLoff / 10),
                        0);
          osDelay(10);
        }
        break;
      }
      case SIG_LED_TRIGGERB: {
        printf_LED("&LED\tGet trigger B message\n");
        // printf_LED("&No oprat should do\n");
        multi_trigger_func(SYS_CFG.TBMode);
        jump_flag = 1;
        evt.value.v = SIG_LED_INTORUN;
        goto TAG;
      }
      case SIG_LED_TRIGGERC: {
        printf_LED("&LED\tGet trigger C message\n");
        multi_trigger_func(SYS_CFG.TCMode);
        jump_flag = 1;
        evt.value.v = SIG_LED_INTORUN;
        goto TAG;
      }
      case SIG_LED_TRIGGERD: {
        printf_LED("&LED\tGet trigger D message\n");
        multi_trigger_func(SYS_CFG.TDMode);
        jump_flag = 1;
        evt.value.v = SIG_LED_INTORUN;
        goto TAG;
      }
      case SIG_LED_TRIGGERE: {
        printf_LED("&LED\tGet trigger E on message\n");
				if (multi_trigger_func(SYS_CFG.TEMode)) {
					LED_COLOR_SET(RGB_PROFILE[(sBANK + shift_bank) % nBank][1], 0xFF, 1);
					trigger_e = IN_TRIGGER_E;
					flag = 0;
					break;
				}
      }
      case SIG_LED_TRIGGEREOFF: {
        printf_LED("&LED\tGet trigger E off message\n");
        LED_COLOR_SET(RGB_PROFILE[(sBANK + shift_bank) % nBank][0], 0xFF, 1);
        trigger_e = OUT_TRIGGER_E;
        break;
      }
      case SIG_LED_SWITCHBANK: {
        printf_LED("&LED\tGet switch bank\n");
        shift_bank += 1;
        shift_bank %= nBank;
        jump_flag = 1;
        evt.value.v = SIG_LED_INTORUN;
        goto TAG;
        /*
        // Power off LED first
        {
          uint16_t cnt;
          printf_LED("&LED\tGet mode ready message\n");
          cnt = SYS_CFG.TLoff / 10;
          while (cnt--) {
            LED_COLOR_SET(RGB_PROFILE[0][0], 0xFF * cnt / (SYS_CFG.TLoff / 10),
                          0);
            osDelay(10);
          }
        }
        {
          uint16_t cnt;
          printf_LED("&LED\tGet mode running message\n");
          for (cnt = 1; cnt <= SYS_CFG.TLon / 10; cnt++) {
            LED_COLOR_SET(RGB_PROFILE[(sBANK + shift_bank) % nBank][0],
                          0xFF * cnt / (SYS_CFG.TLon / 10), 1);
            osDelay(10);
          }
        }
      } break;
      // Power up LED then
      case SIG_LED_CHARGEA: {
        printf_LED("&LED\tGet Charge A\n");
        RGBL r = {1023, 0, 0, 0};
        uint8_t i;
        while (1) {
          for (i = 0; i < 20; i++) {
            LED_COLOR_SET(r, i, 2);
            evt = osMessageGet(SIG_LEDHandle, 125);
            if (evt.status == osEventMessage) goto TAG;
          }
          for (i = 0; i < 20; i++) {
            LED_COLOR_SET(r, 20 - i, 2);
            evt = osMessageGet(SIG_LEDHandle, 125);
            if (evt.status == osEventMessage) goto TAG;
          }
        }
      }
      */

      case SIG_LED_CHARGEB: {
        printf("&LED\tGet Charge B\n");
        RGBL r = {0, 0, 1023, 0};
        uint8_t i;
        while (1) {
          for (i = 0; i < 20; i++) {
            LED_COLOR_SET(r, i, 2);
            evt = osMessageGet(SIG_LEDHandle, 125);
            if (evt.status == osEventMessage) goto TAG;
          }
          for (i = 0; i < 20; i++) {
            LED_COLOR_SET(r, 20 - i, 2);
            evt = osMessageGet(SIG_LEDHandle, 125);
            if (evt.status == osEventMessage) goto TAG;
          }
        }
      }
      default: {
        printf_LED("&LED\tGet undefine SIG of LED:%d\n", evt.value.v);
        break;
      }
    }
  }
}

void LED_COLOR_SET(RGBL data, uint8_t DC, uint8_t mode) {
  if (mode == 1) {
    TIM1->CCR1 = (uint32_t)data.R * SYS_CFG.Lbright * DC / 0xFF / 1024;
    TIM1->CCR2 = (uint32_t)data.G * SYS_CFG.Lbright * DC / 0xFF / 1024;
    TIM1->CCR3 = (uint32_t)data.B * SYS_CFG.Lbright * DC / 0xFF / 1024;
    TIM1->CCR4 = (uint32_t)data.L * SYS_CFG.Lbright * DC / 0xFF / 1024;
  } else if (mode == 0) {
    TIM1->CCR1 = TIM1->CCR1 * DC / 0xFF;
    TIM1->CCR2 = TIM1->CCR2 * DC / 0xFF;
    TIM1->CCR3 = TIM1->CCR3 * DC / 0xFF;
    TIM1->CCR4 = TIM1->CCR4 * DC / 0xFF;
  } else if (mode == 2) {
    TIM1->CCR1 = (uint32_t)data.R * DC / 0xFF;
    TIM1->CCR2 = (uint32_t)data.G * DC / 0xFF;
    TIM1->CCR3 = (uint32_t)data.B * DC / 0xFF;
    TIM1->CCR4 = (uint32_t)data.L * DC / 0xFF;
  }
}

void Simple_LED_Opt(void) {
  extern Simple_LED SL[nBank][256];
  extern Simple_LED_T SLT[nBank];
  extern Simple_LED_STEP SLS[nBank];
  extern uint8_t sBANK;
  static Simple_LED_STEP cnt = 0;

  HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin,
                    (GPIO_PinState)SL[sBANK][cnt].LED1);
  HAL_GPIO_WritePin(LED6_GPIO_Port, LED6_Pin,
                    (GPIO_PinState)SL[sBANK][cnt].LED2);
  HAL_GPIO_WritePin(LED7_GPIO_Port, LED7_Pin,
                    (GPIO_PinState)SL[sBANK][cnt].LED3);
  HAL_GPIO_WritePin(LED8_GPIO_Port, LED8_Pin,
                    (GPIO_PinState)SL[sBANK][cnt].LED4);
  /* New LED Port */
  HAL_GPIO_WritePin(LED9_GPIO_Port, LED9_Pin,
                    (GPIO_PinState)SL[sBANK][cnt].LED5);
  HAL_GPIO_WritePin(LED10_GPIO_Port, LED10_Pin,
                    (GPIO_PinState)SL[sBANK][cnt].LED6);
  HAL_GPIO_WritePin(LED11_GPIO_Port, LED11_Pin,
                    (GPIO_PinState)SL[sBANK][cnt].LED7);
  HAL_GPIO_WritePin(LED12_GPIO_Port, LED12_Pin,
                    (GPIO_PinState)SL[sBANK][cnt].LED8);
  cnt += 1;
  cnt %= SLS[sBANK];
}

static int multi_trigger_func(uint32_t mode) {
  uint8_t real_mode = 0;
  {
    uint8_t cnt = 0;
    uint8_t r = 0;
    uint8_t t = 4;
    switch (mode) {
      case 0x00: cnt = 0; break;

      case 0x01:
      case 0x02:
      case 0x04:
      case 0x08: cnt = 1; break;

      case 0x03:
      case 0x05:
      case 0x06:
      case 0x09:
      case 0x0A:
      case 0x0C: cnt = 2; break;

      case 0x0E:
      case 0x0D:
      case 0x0B:
      case 0x07: cnt = 3; break;

      case 0x0F: cnt = 4; break;
    }
    r = rand() % cnt + 1;
    while (t--) {
			real_mode <<= 1;
      if (mode & 0x08 && !(--r)) {
        real_mode |= 1;
      }
      mode <<= 1;
    }
		
		t = 4;
		cnt = 0;
		while (t--) {
			if (real_mode & 0x01) {
				break;
			}
			cnt++;
			real_mode >>= 1;
		}
		
		real_mode = cnt + 1;
  }
  switch (real_mode) {
    case 2:
      LED_COLOR_SET(RGB_PROFILE[(sBANK + shift_bank) % nBank][1], 0xFF, 1);
      osDelay(T_Spark /*SYS_CFG.TDflip*/);
      LED_COLOR_SET(RGB_PROFILE[(sBANK + shift_bank) % nBank][0], 0xFF, 1);
      break;
    case 3:{
      uint32_t cnt = nSparkCount;
      while (cnt--) {
        LED_COLOR_SET(RGB_PROFILE[(sBANK + shift_bank) % nBank][1], 0xFF, 1);
        osDelay(T_Spark /*SYS_CFG.TDflip*/);
        LED_COLOR_SET(RGB_PROFILE[(sBANK + shift_bank) % nBank][0], 0xFF, 1);
        osDelay(T_nSparkGap);
      }
		} break;
    
    case 4:
    return 1;

    default:
    // Not surpport mode
    return 0;
  }
	return 0;
}
