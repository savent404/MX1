/*
 * Created on Fri Mar 03 2017
 *
 * The MIT License (MIT)
 * Copyright (c) 2017 savent gate
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "ACC.h"

typedef struct {
    float true_data;
    float diff_data;
    int8_t logic_data;
} ACC_DataStructureTypedef;

#define AC 2
#define BC 0
#define TOTAL 7

static ACC_DataStructureTypedef AccData[TOTAL];
static uint32_t AccPos = 0;
/**
 * @Breif: Normalize x,y,z to one float data
 */
__inline static float ACC_Normalize(int16_t x, int16_t y, int16_t z);
/**
 * @Breif: Count Logic point number
 * @Para:  opt=0, Counting true zero through, or Counting fake zero through
 */
__inline static uint32_t ACC_SUM(uint8_t opt);
/**
 * @Breif: Dtat Insert In AccData
 * @Para: 0->Normal, 1->[1,6], 2->[0]
 */
__inline static void ACC_Insert(float data, uint8_t init_flag);
/**
 * @Breif: check trigger type
 * @Retvl: return trigger type
 */
ACC_TriggerTypedef ACC_TriggerCheck(void)
{
    ACC_TriggerTypedef res;
    Lis3dData _data;
    float data;

    taskENTER_CRITICAL();
    Lis3dGetData(&_data);
    taskEXIT_CRITICAL();

    // Normalize
    data = ACC_Normalize(_data.x, _data.y, _data.z);

    // Insert data 
    {
        static uint8_t init_flag = 1;
        ACC_Insert(data, init_flag);
        if (init_flag) {
            init_flag = 0;
        }
    }
    

    // Check if is Trigger C, or set to TriggerB
    if (ACC_SUM(0) > AC) {
        res = ACC_TriggerC;
    } else if (abs(data) > 0.99) {
        res = ACC_TriggerC;
    }

    // Check if is Trigger B, or set to TriggerNONE
    else if (ACC_SUM(1) > BC) {
        res = ACC_TriggerNONE;
    } else {
        res = ACC_TriggerB;
    }

    return res;

}

/**
 * @Breif: Normalize x,y,z to one float data
 */
__inline static float ACC_Normalize(int16_t x, int16_t y, int16_t z) {
    float fx = (float)x * 2.0 / 0xFFFF,
          fy = (float)y * 2.0 / 0xFFFF,
          fz = (float)z * 2.0 / 0xFFFF;
    return sqrt(fx*fx + fy*fy + fz*fz);
}
/**
 * @Breif: Count Logic point number
 * @Para:  opt=0, Counting true zero through, or Counting fake zero through
 */
__inline static uint32_t ACC_SUM(uint8_t opt) {
    uint32_t cnt = TOTAL;
    uint32_t res = 0;
    if (!opt) {
        while (cnt--)
        {
            if (AccData[cnt].logic_data == 1)
                res += 1;
        } return res;
    } else {
        while (cnt--)
        {
            if (AccData[cnt].logic_data == -1)
                res += 1;
        } return res;
    }
}

__inline static void ACC_Insert(float data, uint8_t init_flag) {
    AccData[AccPos].true_data = data;
    AccData[AccPos].diff_data = data - AccData[(AccPos+TOTAL-1)%TOTAL].true_data;
    if (AccData[AccPos] * AccData[(AccPos+TOTAL-1)%TOTAL] < 0) {
        if (abs(data) > 0.1) {
            AccData[AccPos].logic_data = 1;
        } else {
            AccData[AccPos].logic_data = -1;
        }
    } else {
        AccData[AccPos].logic_data = 0;
    }

    if (init_flag) {
        AccData[AccPos].diff_data = 0;
        AccData[AccPos].logic_data = 0;
    }

    AccPos += 1;
    AccPos %= TOTAL;
}
