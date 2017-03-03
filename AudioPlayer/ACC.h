#ifndef _ACC_H_
#define _ACC_H_

#include "cmsis_os.h"
#include "stm32f1xx_hal.h"
#include "Lis3D.h"
#include "math.h"
#include "FreeRTOS.h"

typedef enum { ACC_TriggerNONE, ACC_TriggerB, ACC_TriggerC } ACC_TriggerTypedef;

ACC_TriggerTypedef ACC_TriggerCheck(void);


#endif
