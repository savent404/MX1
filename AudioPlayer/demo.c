#include "ap.h"
#include "af.h"
#include "stm32f1xx_hal.h"
FATFS fs;
FRESULT fres;

FIL file_1, file_2;
UINT file_cnt;
struct _AF_PCM pcm_1, pcm_2;
struct _AF_DATA data_1, data_2;

static uint16_t file_buf[FIFO_SIZ];
static uint16_t extra_buf[FIFO_SIZ];

uint32_t i;

int main(void) {
	fres = f_mount(&fs, "0", 1);
  fres = f_open(&file_1, "/1.wav", FA_READ);
  fres = f_open(&file_2, "/2.wav", FA_READ);
  fres = f_read(&file_1, &pcm_1, sizeof(pcm_1), &file_cnt);
  fres = f_read(&file_1, &data_1, sizeof(data_1), &file_cnt);
	
	fres = f_read(&file_1, file_buf, 2*FIFO_SIZ, &file_cnt);
	fres = f_read(&file_2, extra_buf, 2*FIFO_SIZ, &file_cnt);
	i = AP_IN_MIX(file_buf, extra_buf);
	fres = f_read(&file_1, file_buf, 2*FIFO_SIZ, &file_cnt);
	fres = f_read(&file_2, extra_buf, 2*FIFO_SIZ, &file_cnt);
	i = AP_IN_MIX(file_buf, extra_buf);
	fres = f_read(&file_1, file_buf, 2*FIFO_SIZ, &file_cnt);
	fres = f_read(&file_2, extra_buf, 2*FIFO_SIZ, &file_cnt);
	i = AP_IN_MIX(file_buf, extra_buf);
	fres = f_read(&file_1, file_buf, 2*FIFO_SIZ, &file_cnt);
	fres = f_read(&file_2, extra_buf, 2*FIFO_SIZ, &file_cnt);
	data_1.size -= 2*FIFO_SIZ*4;
	data_2.size -= 2*FIFO_SIZ*4;
	AP_Play();
	while (1)
	{
	/* USER CODE END WHILE */

	/* USER CODE BEGIN 3 */
	if (!AP_IN_MIX(file_buf, extra_buf)) {
		
		if (data_1.size >= 2*FIFO_SIZ) {
			fres = f_read(&file_1, &file_buf, 2*FIFO_SIZ, &file_cnt);
			data_1.size -= 2*FIFO_SIZ;
		}
		else {
			memset(file_buf, 0x00, 2*FIFO_SIZ);
			//extra test
			HAL_Delay(200);
			NVIC_SystemReset();
		}
		
		if (data_2.size >= 2*FIFO_SIZ) {
			fres = f_read(&file_2, &extra_buf, 2*FIFO_SIZ, &file_cnt);
			data_2.size -= 2*FIFO_SIZ;
		}
		else
			memset(extra_buf, 0x00, 2*FIFO_SIZ);
	}
  }
}