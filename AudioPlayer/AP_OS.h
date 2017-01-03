#ifndef _APOS_H_
#define _APOS_H_

#include "AF.h"
#include "stdint.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "tim.h"
#include "dac.h"

#define osFIFO_SIZE 256
#define osFIFO_NUM  3
__inline __weak uint16_t convert_single(uint16_t src);
__inline __weak uint16_t convert_double(uint16_t src_1, uint16_t src_2);

/* When System from Close into Ready */
extern const uint8_t SIG_AUDIO_STARTUP;

/* When System from Ready into Close */
extern const uint8_t SIG_AUDIO_POWEROFF;

/* When SYstem from Ready into running */
extern const uint8_t SIG_AUDIO_INTORUN;

/* When System from Running into Reayd */
extern const uint8_t SIG_AUDIO_OUTRUN;

extern osThreadId    WAV_CTLHandle;
extern osThreadId    DAC_CTLHandle;
extern osSemaphoreId DMA_FLAGHandle;
extern osMessageQId  pWAVHandle;
extern osMessageQId  SIG_PLAYWAVHandle;

#endif
