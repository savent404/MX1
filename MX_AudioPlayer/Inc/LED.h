#ifndef _LED_H_
#define _LED_H_

#include "stm32f1xx.h"
typedef enum {LED1, LED2, LED3, LED4, LED5, LED6, LED7, LED8}LEDs;

void LED_switch(LEDs, GPIO_PinState stat);
#endif
