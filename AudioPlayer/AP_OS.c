#include "AP_OS.h"
#include "fatfs.h"
/* When System from Close into Ready */
const uint32_t SIG_AUDIO_STARTUP = 0x10000000;
/* When System from Ready into Close */
const uint32_t SIG_AUDIO_POWEROFF= 0x20000000;
/* When SYstem from Ready into running */
const uint32_t SIG_AUDIO_INTORUN = 0x40000000;
/* When System from Running into Reayd */
const uint32_t SIG_AUDIO_OUTRUN  = 0x80000000;

//Wav's PCM structure showing about PCM data
static struct _AF_PCM pcm1, pcm2;
//Wav's SIZE and CHANNELs number
static struct _AF_DATA data1, data2;
//Used when open a file, tow channels.
static FIL file_1, file_2;
//Used when read buffer from a file, tow channels
static UINT cnt_1, cnt_2;
//wave pcm buffer, tow channels
static uint16_t audio_buf1[osFIFO_NUM][osFIFO_SIZE],
                audio_buf2[osFIFO_NUM][osFIFO_SIZE];

/*  Waiting for CMD form another Handle
    * Play single Wav file
    * Play double Wav file
*/
void WAVHandle(void const * argument)
{
  uint8_t pos1, pos2;
  osEvent evt;
  for(;;)
  {
    evt = osSignalWait(0, 10);
    if (evt.status != osEventSignal)
    continue;

    /* search cmd */
    switch(evt.value.v) {
      //When System from close into ready.
      case SIG_AUDIO_STARTUP:
      if (f_open(&file_1, "0:/System/Boot.wav", FA_READ) != FR_OK)
      break;
      f_read(&file_1, &pcm1, sizeof(pcm1), &cnt_1);
      f_read(&file_1, &data1, sizeof(data1), &cnt_1);
      data1.size -= sizeof(pcm1) + sizeof(data1);
      pos1 = 0;
      while (1) {
        /* No enuough buffer, exit */
        if (data1.size < osFIFO_SIZE)
        break;
        /* Read buffer and waiting for put succese. */
        f_read(&file_1, audio_buf1[pos1], osFIFO_SIZE, &cnt_1);
        data1.size -= cnt_1;
        osMessagePut(pWAVHandle, (uint32_t)audio_buf1[pos1], 0);
        pos1 += 1;
        pos1 %= osFIFO_NUM;
      }
      f_close(&file_1);
      break;

      //When System from ready into close
      case SIG_AUDIO_POWEROFF:
      if (f_open(&file_1, "0:/System/poweoff.wav", FA_READ) != FR_OK)
      break;
      f_read(&file_1, &pcm1, sizeof(pcm1), &cnt_1);
      f_read(&file_1, &data1, sizeof(data1), &cnt_1);
      data1.size -= sizeof(pcm1) + sizeof(data1);
      pos1 = 0;
      while (1) {
        /* No enuough buffer, exit */
        if (data1.size < osFIFO_SIZE)
        break;
        /* Read buffer and waiting for put succese. */
        f_read(&file_1, audio_buf1[pos1], osFIFO_SIZE, &cnt_1);
        data1.size -= cnt_1;
        osMessagePut(pWAVHandle, (uint32_t)audio_buf1[pos1], 0);
        pos1 += 1;
        pos1 %= osFIFO_NUM;
      }
      f_close(&file_1);
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
    evt = osMessageGet(pWAVHandle, 10);
    if (evt.status == osOK) {
      flag = stopped;
      HAL_TIM_Base_Stop(&htim2);
      HAL_GPIO_WritePin(Audio_EN_GPIO_Port, Audio_EN_Pin, GPIO_PIN_RESET);
      HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
      continue;
    }

    else if (evt.status == osEventMessage) {

      //Waiting for DAC DMA ConvCpltCall
      if (flag == running) {
        osSemaphoreWait(DMA_FLAGHandle, osWaitForever);
        HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)evt.value.p, osFIFO_SIZE, DAC_ALIGN_12B_R);
      }
      //restart DAC DMA mode
      else {
        osSemaphoreWait(DMA_FLAGHandle, osWaitForever);
        HAL_TIM_Base_Start(&htim2);
        HAL_GPIO_WritePin(Audio_EN_GPIO_Port, Audio_EN_Pin, GPIO_PIN_SET);
        HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)evt.value.p, osFIFO_SIZE, DAC_ALIGN_12B_R);
      }
    }
  }
}
void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hdac) {
  osSemaphoreRelease(DMA_FLAGHandle);
}
