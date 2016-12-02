#ifndef _AP_H_
#define _AP_H_
/* This is Audio Player Header file
 */
#include "AF.h"
#include "stdint.h"
#ifdef __cplusplus
extern "C" {
#endif

// Usr Type Define
#define FIFO_NUM 3
#define FIFO_SIZ 1024

enum {running, empty} Player_status = empty;

/* Audio FIFO structure
 */
struct {
    uint16_t*
    src[FIFO_NUM];
    /* FIFO status */
    uint8_t FIFO_S;
    uint8_t FIFO_cnt;
}Audio_FIFO = {
    .FIFO_S = 0,
    .FIFO_cnt = 0,
};

/* Aduio FIFO operation
 */
// Add a node in FIFO
int8_t    AF_IN(uint16_t *src);

// Return a node in FIFO
uint16_t* AF_OUT(void);

// Play
int8_t    AF_Play(void);

/* Usr add in proj
 */
void Player_Triger(void);

#ifdef _cplusplus
}
#endif

#endif//end of file
