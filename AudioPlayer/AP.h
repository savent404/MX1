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
#define FIFO_SIZ 256



/* Aduio FIFO operation
 */
// Add a node in FIFO
int8_t    AP_IN(uint16_t *src);

int8_t    AP_IN_MIX(uint16_t *src_1, uint16_t *src_2);

// Return a node in FIFO
uint16_t* AP_OUT(void);

// Play
int8_t    AP_Play(void);

int8_t    AP_Stop(void);

/* Usr add in proj
 */
void AP_Triger(void);

__weak uint16_t  convert_single(uint16_t src);
__weak uint16_t  convert_double(uint16_t src_1, uint16_t src_2);

#ifdef _cplusplus
}
#endif

#endif//end of file
