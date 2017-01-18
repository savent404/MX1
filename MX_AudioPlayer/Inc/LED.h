#ifndef _LED_H_
#define _LED_H_

#include "stm32f1xx.h"
#include "cmsis_os.h"
#include "USR_CFG.H"
#include "DEBUG_CFG.h"
#include "gpio.h"
#include "tim.h"

typedef enum { LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8 } LEDs;

/* When System from Close into Ready */
extern const uint8_t SIG_LED_STARTUP;

/* When System from Ready into Close */
extern const uint8_t SIG_LED_POWEROFF;

/* When SYstem from Ready into running */
extern const uint8_t SIG_LED_INTORUN;

/* When System from Running into Reayd */
extern const uint8_t SIG_LED_OUTRUN;

/* When System in Ready switch the Bank */
extern const uint8_t SIG_LED_BANKSWITCH;

/* Triggers */
/* When System in Running, Trigger B */
extern const uint8_t SIG_LED_TRIGGERB;
/* When System in Running, Trigger C */
extern const uint8_t SIG_LED_TRIGGERC;
/* When System in Running, Trigger D */
extern const uint8_t SIG_LED_TRIGGERD;
/* When System in Running, Trigger E */
extern const uint8_t SIG_LED_TRIGGERE;
/* When System in Running, Trigger E off */
extern const uint8_t SIG_LED_TRIGGEREOFF;
/* When charge power .ing */
extern const uint8_t SIG_LED_CHARGEA;
/* WHen charge power .done */
extern const uint8_t SIG_LED_CHARGEB;

extern const uint8_t SIG_LED_SWITCHBANK;

typedef struct {
  int R;
  int G;
  int B;
  int L;
}RGBL;

typedef struct {
	uint8_t LED1:1;
	uint8_t LED2:1;
	uint8_t LED3:1;
	uint8_t LED4:1;
}Simple_LED;
typedef int Simple_LED_T;
typedef int Simple_LED_STEP;
void LED_COLOR_SET(RGBL data, uint8_t DC, uint8_t mode);
void Simple_LED_Opt(void);
#endif
