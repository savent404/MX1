#include "AP.h"
#include "stm32f1xx_hal.h"

extern DAC_HandleTypeDef hdac;

struct sfifo {
  enum {empty, full} fifo_status;
  uint16_t fifo[FIFO_SIZ];
};
static uint8_t dma_triger = 0;
static enum {running, pause} AP_status = pause;
static struct {
  struct sfifo fifo[FIFO_NUM];
  uint8_t start;
  uint8_t end;
}SFIFO = {
  .fifo = {empty},
  .start = 0,
  .end   = 0
};

/* Aduio FIFO operation
 */
// Add a node in FIFO
int8_t AP_IN(uint16_t *src) {
  // check if FIFO is full return -1
  uint16_t pos = SFIFO.end;
  uint32_t i;
  uint16_t *pt;
  if (SFIFO.fifo[pos].fifo_status == full)
  return -1;

  //then you can add in
  pt = SFIFO.fifo[pos].fifo;
  for (i = 0; i < FIFO_SIZ; i++) {
    *pt = convert_single(*src);
    pt++;
    src++;
  }
  SFIFO.fifo[pos].fifo_status = full;
  SFIFO.end += 1;
  SFIFO.end %= FIFO_NUM;
  return 0;
}

int8_t AP_IN_MIX(uint16_t *src_1, uint16_t *src_2) {
  // check if FIFO is full return -1
  uint16_t pos = SFIFO.end;
  uint32_t i;
  uint16_t *pt;
  if (SFIFO.fifo[pos].fifo_status == full)
  return -1;

  //then you can add in
  pt = SFIFO.fifo[pos].fifo;
  for (i = 0; i < FIFO_SIZ; i++) {
    *pt = convert_double(*src_1, *src_2);
    pt++;
    src_1++;
		src_2++;
  }
  SFIFO.fifo[pos].fifo_status = full;
  SFIFO.end += 1;
  SFIFO.end %= FIFO_NUM;
  return 0;
}

// Return a node in FIFO
uint16_t* AP_OUT(void) {
  uint16_t pos = SFIFO.start;
  uint16_t *pt = 0;
  // if fifo is empty
  if (SFIFO.fifo[pos].fifo_status == empty)
  return pt;

  //then you can get
  pt = SFIFO.fifo[pos].fifo;
  SFIFO.fifo[pos].fifo_status = empty;
  SFIFO.start += 1;
  SFIFO.start %= FIFO_NUM;
  return pt;

}

// Play
int8_t  AP_Play(void) {
  uint16_t *pt = 0;
	extern TIM_HandleTypeDef htim2;
  if (AP_status == pause) {
		HAL_TIM_Base_Start(&htim2);
    HAL_GPIO_WritePin(Audio_EN_GPIO_Port, Audio_EN_Pin, GPIO_PIN_SET);
    AP_status = running;
  }

  pt = AP_OUT();
  if (pt ==0)
  return -1;

  HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)pt, FIFO_SIZ, DAC_ALIGN_12B_R);
  return 0;
}

int8_t  AP_Stop(void) {
  HAL_GPIO_WritePin(Audio_EN_GPIO_Port, Audio_EN_Pin, GPIO_PIN_RESET);
  AP_status = pause;
  HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
  return 0;
}

/* Usr add in proj
 */
void AP_Triger(void) {
  if (dma_triger && AP_status == running)
  AP_Play();
	dma_triger = 0;
}
void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hdac) {
  dma_triger = 1;
	AP_Triger();
}
__inline __weak uint16_t  convert_single(uint16_t src) {
  return (src + 0x7FFF) >> 4;
}
__inline __weak uint16_t  convert_double(uint16_t src_1, uint16_t src_2) {
	src_1 += 0x7FFF;
	src_2 += 0x7FFF;
	src_1 >>= 4;
	src_2 >>= 4;
	return (src_1 + src_2)/2;
}
