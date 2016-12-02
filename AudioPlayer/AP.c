/* This is Audio Player Source file
 */
#include "AP.h"
#include "stm32f1xx_hal.h"

extern DAC_HandleTypeDef hdac;


// Add a node in FIFO
int8_t AF_IN(uint16_t *src) {
    if (Audio_FIFO.FIFO_cnt < FIFO_NUM) {
        Audio_FIFO.src[(Audio_FIFO.FIFO_S + Audio_FIFO.FIFO_cnt - 1)%FIFO_NUM] = src;
        Audio_FIFO.FIFO_cnt += 1;
        return 0;
    } return -1;
}

// Return a node in FIFO
uint16_t *AF_OUT(void) {
    uint8_t pos;
    if (Audio_FIFO.FIFO_cnt > 0) {
        pos = Audio_FIFO.FIFO_S;
        Audio_FIFO.FIFO_cnt -= 1;
        Audio_FIFO.FIFO_S += 1;
        Audio_FIFO.FIFO_S %= FIFO_NUM;
        return Audio_FIFO.src[pos];
    } return 0;
}

// Play
int8_t AF_Play(void) {
    uint32_t *src = 0;
    if (!(Audio_FIFO.FIFO_cnt)) return -1;

    src = (uint32_t*)AF_OUT();
    if (src == 0) return -1;
    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, src, FIFO_SIZ, DAC_ALIGN_12B_R);
    Player_status = running;
    return 0;
}
// DMA callback
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef* hdac) {
    Player_status = empty;
}

// Timer triger
void Player_Triger(void) {
    if(Player_status != running) {
        // try play
        AF_Play();
    }
}
