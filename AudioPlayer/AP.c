/* This is Audio Player Source file
 */
#include "AP.h"

// Init FIFO status
void AF_INIT(Audio_FIFO *pt) {
    pt->FIFO_S = 0;
    pt->FIFO_cnt = 0;
}

// Add a node in FIFO
int8_t AF_IN(Audio_FIFO *pt, uint16_t *src) {
    if (pt->FIFO_cnt < FIFO_NUM) {
        pt->src[(pt->FIFO_S + FIFO_cnt - 1)%FIFO_NUM] = src;
        return 0;
    }
    return -1;
}

// Return a node in FIFO
uint16_t *AF_OUT(Audio_FIFO *pt) {
    uint8_t pos;
    if (pt->FIFO_cnt > 0) {
        pos = pt->FIFO_S;
        pt->FIFO_cnt -= 1;
        pt->FIFO_S += 1;
        pt->FIFO_S %= FIFO_NUM;
        return pt->src[pos];
    }
    return 0;
}