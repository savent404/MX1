#include "LED.h"
void LED_switch(LEDs led, GPIO_PinState stat) {
	switch (led) {
		case LED1:	HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, stat); break;
		case LED2:  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, stat); break;
		case LED3:	HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, stat); break;
		case LED4:	HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, stat); break;
		case LED5:	HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin, stat); break;
		case LED6:	HAL_GPIO_WritePin(LED6_GPIO_Port, LED6_Pin, stat); break;
		case LED7:	HAL_GPIO_WritePin(LED7_GPIO_Port, LED7_Pin, stat); break;
		case LED8:	HAL_GPIO_WritePin(LED8_GPIO_Port, LED8_Pin, stat); break;
	}
}
