#include "AP_OS.h"
#include "fatfs.h"
/* When System from Close into Ready */
const uint8_t SIG_AUDIO_STARTUP = 0x0010;
/* When System from Ready into Close */
const uint8_t SIG_AUDIO_POWEROFF= 0x0020;
/* When SYstem from Ready into running */
const uint8_t SIG_AUDIO_INTORUN = 0x0040;
/* When System from Running into Reayd */
const uint8_t SIG_AUDIO_OUTRUN  = 0x0080;


//wave pcm buffer, tow channels
static uint16_t audio_buf1[osFIFO_NUM][osFIFO_SIZE],
                audio_buf2[osFIFO_NUM][osFIFO_SIZE];
static FRESULT fres;


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
/*  Waiting for CMD form another Handle
    * Play single Wav file
    * Play double Wav file
*/
void WAVHandle(void const * argument)
{
  uint8_t pos1, pos2;
  osEvent evt;
	//Wav's PCM structure showing about PCM data
	static struct _AF_PCM pcm1, pcm2;
	//Wav's SIZE and CHANNELs number
	static struct _AF_DATA data1, data2;
	//Used when open a file, tow channels.
	static FIL file_1, file_2;
	//Used when read buffer from a file, tow channels
	static UINT cnt_1, cnt_2;
	static UINT i;
	static uint16_t * fpt_1, *fpt_2;
	static osStatus osstatus;
  for(;;)
  {
    evt = osMessageGet(SIG_PLAYWAVHandle, 10);
    if (evt.status != osEventMessage)
    continue;

    /* search cmd */
    switch(evt.value.v) {
      //When System from close into ready.
      case SIG_AUDIO_STARTUP:
			taskENTER_CRITICAL();
			fres = f_open(&file_1, "0:/System/Boot.wav", FA_READ);
			taskEXIT_CRITICAL();
      if (fres != FR_OK){
				printf("Open file Error:%d\n", fres);
				break;
			}
      fres = f_read(&file_1, &pcm1, sizeof(pcm1), &cnt_1);
      fres = f_read(&file_1, &data1, sizeof(data1), &cnt_1);
      pos1 = 0;
      while (1) {
        /* No enuough buffer, exit */
        if (data1.size < osFIFO_SIZE)
        break;
				
				if (fres != FR_OK) {
					printf("Something wrong with FATFS. code:%d\n", fres);
					f_close(&file_1);
					break;
				}
        /* Read buffer and waiting for put succese. */
				taskENTER_CRITICAL();
        fres = f_read(&file_1, audio_buf1[pos1], osFIFO_SIZE*2, &cnt_1);
				taskEXIT_CRITICAL();
				fpt_1 = audio_buf1[pos1];
				for (i = 0; i < osFIFO_SIZE; i++) {
					*fpt_1 = convert_single(*fpt_1);
					fpt_1++;
				}
        data1.size -= cnt_1;
        osstatus = osMessagePut(pWAVHandle, (uint32_t)audio_buf1[pos1], 0);
				while (osstatus) {
					osstatus = osMessagePut(pWAVHandle, (uint32_t)audio_buf1[pos1], 0);
				}
				//printf("Read a buffer:%d \t last:%d\tosstatus = %d\n", cnt_1, data1.size, osstatus);
        pos1 += 1;
        pos1 %= osFIFO_NUM;
      }
			taskENTER_CRITICAL();
      fres = f_close(&file_1);
			taskEXIT_CRITICAL();
			printf("a flie closed\n");
			//osSignalClear(WAV_CTLHandle, SIG_AUDIO_STARTUP);
      break;

      //When System from ready into close
      case SIG_AUDIO_POWEROFF:
			taskENTER_CRITICAL();
			fres = f_open(&file_1, "0:/System/Poweroff.wav", FA_READ);
			taskEXIT_CRITICAL();
      if (fres != FR_OK){
				printf("Open file Error:%d\n", fres);
				break;
			}
      fres = f_read(&file_1, &pcm1, sizeof(pcm1), &cnt_1);
      fres = f_read(&file_1, &data1, sizeof(data1), &cnt_1);
      pos1 = 0;
      while (1) {
        /* No enuough buffer, exit */
        if (data1.size < osFIFO_SIZE)
        break;
				
				if (fres != FR_OK) {
					printf("Something wrong with FATFS. code:%d\n", fres);
					f_close(&file_1);
					break;
				}
        /* Read buffer and waiting for put succese. */
				taskENTER_CRITICAL();
        fres = f_read(&file_1, audio_buf1[pos1], osFIFO_SIZE*2, &cnt_1);
				taskEXIT_CRITICAL();
				fpt_1 = audio_buf1[pos1];
				for (i = 0; i < osFIFO_SIZE; i++) {
					*fpt_1 = convert_single(*fpt_1);
					fpt_1++;
				}
        data1.size -= cnt_1;
        osstatus = osMessagePut(pWAVHandle, (uint32_t)audio_buf1[pos1], 0);
				while (osstatus) {
					osstatus = osMessagePut(pWAVHandle, (uint32_t)audio_buf1[pos1], 0);
				}
				//printf("Read a buffer:%d \t last:%d\tosstatus = %d\n", cnt_1, data1.size, osstatus);
        pos1 += 1;
        pos1 %= osFIFO_NUM;
      }
			taskENTER_CRITICAL();
      fres = f_close(&file_1);
			taskEXIT_CRITICAL();
			printf("a flie closed\n");
      break;

      //When System from Ready into running.
      case SIG_AUDIO_INTORUN:
      break;

      //When System from Running into Ready.
      case SIG_AUDIO_OUTRUN:
      break;

      //case ...
      default:
      break;
    }
  }
}

/* Waiting for Message Queue.
   If no more Message Queue, Stop DAC output
*/
void DACHandle(void const * argument)
{
  osEvent evt;
  static  enum {running, stopped} flag = stopped;
  for(;;)
  {
    evt = osMessageGet(pWAVHandle, 59);
    if (evt.status == osEventTimeout) {
			if (flag == stopped)
			continue;
			printf("Stop DMA\n");
      flag = stopped;
      HAL_GPIO_WritePin(Audio_EN_GPIO_Port, Audio_EN_Pin, GPIO_PIN_RESET);
      continue;
    }

    else if (evt.status == osEventMessage) {
      //Waiting for DAC DMA ConvCpltCall
      if (flag == running) {
        HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)evt.value.p, osFIFO_SIZE, DAC_ALIGN_12B_R);
				osSemaphoreWait(DMA_FLAGHandle, osWaitForever);
      }
      //restart DAC DMA mode
      else {
				printf("Start DMA\n");
				flag = running;
        HAL_TIM_Base_Start(&htim2);
        HAL_GPIO_WritePin(Audio_EN_GPIO_Port, Audio_EN_Pin, GPIO_PIN_SET);
        HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)evt.value.p, osFIFO_SIZE, DAC_ALIGN_12B_R);
				osSemaphoreWait(DMA_FLAGHandle, osWaitForever);
      }
    }
  }
}
void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hdac) {
	static uint32_t CNT;
  osSemaphoreRelease(DMA_FLAGHandle);
	//printf("DMA call back hit:%d\n", ++CNT);
}
