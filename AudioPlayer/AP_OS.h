#ifndef _APOS_H_
#define _APOS_H_

#include "AF.h"
#include "stdint.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "tim.h"
#include "dac.h"

#define osFIFO_SIZE 128
#define osFIFO_NUM 5
__inline __weak uint16_t convert_single(uint16_t src);
__inline __weak uint16_t convert_double(int16_t src_1, int16_t src_2);

/* When System from Close into Ready */
extern const uint8_t SIG_AUDIO_STARTUP;

/* When System from Ready into Close */
extern const uint8_t SIG_AUDIO_POWEROFF;

/* When SYstem from Ready into running */
extern const uint8_t SIG_AUDIO_INTORUN;

/* When System from Running into Reayd */
extern const uint8_t SIG_AUDIO_OUTRUN;
/* When System in Ready switch the Bank */
extern const uint8_t SIG_AUDIO_BANKSWITCH;

/* Triggers */
/* When System in Running, Trigger B */
extern const uint8_t SIG_AUDIO_TRIGGERB;
/* When System in Running, Trigger C */
extern const uint8_t SIG_AUDIO_TRIGGERC;
/* When System in Running, Trigger D */
extern const uint8_t SIG_AUDIO_TRIGGERD;
/* When System in Running, Trigger E */
extern const uint8_t SIG_AUDIO_TRIGGERE;
/* When System in Running, Trigger E off */
extern const uint8_t SIG_AUDIO_TRIGGEREOFF;
/* When LED change bank */
extern const uint8_t SIG_AUDIO_COLORSWITCH;
/* Warnning Low Power */
extern const uint8_t SIG_AUDIO_LOWPOWER;
/* Warnning restart */
extern const uint8_t SIG_AUDIO_RESTART;
/* When System in Running , exit with mute */
extern const uint8_t SIG_AUDIO_OUTRUN_MUTE;
/* Warnning power charge */
extern const uint8_t SIG_AUDIO_CHARGE;
/* 3D list Log output　*/
extern const uint8_t SIG_FATFS_LOG;

extern osThreadId WAV_CTLHandle;
extern osThreadId DAC_CTLHandle;
extern osSemaphoreId DMA_FLAGHandle;
extern osMessageQId pWAVHandle;
extern osMessageQId SIG_PLAYWAVHandle;

#endif
