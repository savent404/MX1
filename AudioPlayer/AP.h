#ifndef _AP_H_
#define _AP_H_
/* This is Audio Player Header file
 */
#include "AF.h"

#ifdef __cplusplus
extern "C" {
#endif

// Usr Type Define
#define FIFO_NUM 3
#define FIFO_SIZ 1024

/* Audio FIFO structure
 */
typedef struct {
    /* FIFO status */
    uint8_t FIFO_S;
    uint8_t FIFO_cnt;
    uint16_t*
    src[FIFO_NUM];
}Audio_FIFO;

/* Aduio FIFO operation
 */

// Init FIFO status
void      AF_INIT(Audio_FIFO *pt);

// Add a node in FIFO
int8_t    AF_IN(Audio_FIFO *pt, uint16_t *src);

// Return a node in FIFO
uint16_t* AF_OUT(Audio_FIFO *pt);


#ifdef _cplusplus
}
#endif

#endif//end of file
