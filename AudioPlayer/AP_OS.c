#include "AP_OS.h"
#include "fatfs.h"
#include "string.h"
#include "USR_CFG.h"
#include "stdlib.h"
/* When System from Close into Ready */
const uint8_t SIG_AUDIO_STARTUP = 0x10;
/* When System from Ready into Close */
const uint8_t SIG_AUDIO_POWEROFF = 0x20;
/* When SYstem from Ready into running */
const uint8_t SIG_AUDIO_INTORUN = 0x40;
/* When System from Running into Ready */
const uint8_t SIG_AUDIO_OUTRUN = 0x80;
/* When System in Ready switch the Bank */
const uint8_t SIG_AUDIO_BANKSWITCH = 0x11;
/* When System in Running, Trigger B */
const uint8_t SIG_AUDIO_TRIGGERB = 0x01;
/* When System in Running, Trigger C */
const uint8_t SIG_AUDIO_TRIGGERC = 0x02;
/* When System in Running, Trigger D */
const uint8_t SIG_AUDIO_TRIGGERD = 0x04;
/* When System in Running, Trigger E */
const uint8_t SIG_AUDIO_TRIGGERE = 0x08;
/* When System in Running, Trigger E off */
const uint8_t SIG_AUDIO_TRIGGEREOFF = 0x09;

// wave pcm buffer, tow channels
static uint16_t audio_buf1[osFIFO_NUM][osFIFO_SIZE],
    audio_buf2[osFIFO_NUM][osFIFO_SIZE];
static FRESULT fres;
static DIR fdir;
static FILINFO finfo;
__inline __weak uint16_t convert_single(uint16_t src) {
  return (src + 0x7FFF) >> 4;
}
__inline __weak uint16_t convert_double(uint16_t src_1, uint16_t src_2) {
  src_1 += 0x7FFF;
  src_2 += 0x7FFF;
  src_1 >>= 4;
  src_2 >>= 4;
  return (src_1 + src_2) / 2;
}

/***
 *** Micro Func Defines */
/* For gods, help my hands */
#define CRITICAL_FUNC(x) \
  taskENTER_CRITICAL();  \
  x;                     \
  taskEXIT_CRITICAL();
/* A normal option to read a file then put to DACHandle
   If only needs to play ONE PCM wave, use it is right.

   @Note: using CRITICAL func to protect SDIO's opt would
          not to be interrupt.
 */
#define NORMAL_READ_PLAY_FILE_1()                                         \
  taskENTER_CRITICAL();                                                   \
  fres = f_read(&file_1, &pcm1, sizeof(pcm1), &cnt_1);                    \
  fres = f_read(&file_1, &data1, sizeof(data1), &cnt_1);                  \
  taskEXIT_CRITICAL();                                                    \
  pos1 = 0;                                                               \
  while (1) {                                                             \
    /* No enuough buffer, exit */                                         \
    if (data1.size < osFIFO_SIZE) break;                                  \
                                                                          \
    if (fres != FR_OK) {                                                  \
      printf("Something wrong with FATFS. code:%d\n", fres);              \
      f_close(&file_1);                                                   \
      break;                                                              \
    }                                                                     \
    /* Read buffer and waiting for put succese. */                        \
    taskENTER_CRITICAL();                                                 \
    fres = f_read(&file_1, audio_buf1[pos1], osFIFO_SIZE * 2, &cnt_1);    \
    taskEXIT_CRITICAL();                                                  \
    fpt_1 = audio_buf1[pos1];                                             \
    for (i = 0; i < osFIFO_SIZE; i++) {                                   \
      *fpt_1 = convert_single(*fpt_1);                                    \
      fpt_1++;                                                            \
    }                                                                     \
    data1.size -= cnt_1;                                                  \
    osstatus = osMessagePut(pWAVHandle, (uint32_t)audio_buf1[pos1], 0);   \
    while (osstatus) {                                                    \
      osstatus = osMessagePut(pWAVHandle, (uint32_t)audio_buf1[pos1], 0); \
    }                                                                     \
    pos1 += 1;                                                            \
    pos1 %= osFIFO_NUM;                                                   \
  }

/*  Waiting for CMD form another Handle
    * Play single Wav file
    * Play double Wav file
*/
void WAVHandle(void const* argument) {
  uint8_t pos1, pos2;
  osEvent evt;
  // Wav's PCM structure showing about PCM data
  static struct _AF_PCM pcm1, pcm2;
  // Wav's SIZE and CHANNELs number
  static struct _AF_DATA data1, data2;
  // Used when open a file, tow channels.
  static FIL file_1, file_2;
  // Used when read buffer from a file, tow channels
  static UINT cnt_1, cnt_2;
  static UINT i;
  static uint16_t *fpt_1, *fpt_2;
  static osStatus osstatus;
  for (;;) {
    evt = osMessageGet(SIG_PLAYWAVHandle, 10);
    if (evt.status != osEventMessage) continue;

    /* search cmd */
    switch (evt.value.v) {
      // When System from close into ready.
      case SIG_AUDIO_STARTUP: {
        printf("@Get startup message\n");
        CRITICAL_FUNC(fres = f_open(&file_1, "0:/System/Boot.wav", FA_READ));
        if (fres != FR_OK) {
          printf("Open file Error:%d\n", fres);
          break;
        }

        NORMAL_READ_PLAY_FILE_1();

        CRITICAL_FUNC(fres = f_close(&file_1));

        printf("a flie closed:%d\n", fres);
      } break;

      // When System from ready into close
      case SIG_AUDIO_POWEROFF: {
        printf("@Get Power off message\n");
        CRITICAL_FUNC(fres =
                          f_open(&file_1, "0:/System/Poweroff.wav", FA_READ));
        if (fres != FR_OK) {
          printf("Open file Error:%d\n", fres);
          break;
        }

        NORMAL_READ_PLAY_FILE_1();

        CRITICAL_FUNC(fres = f_close(&file_1));

        printf("a flie closed:%d\n", fres);
      } break;

      // When System from Ready into running.
      case SIG_AUDIO_INTORUN: {
        // Init Option
        uint8_t loop_flag = 1;
        extern __IO uint8_t sBANK;
        char sdir[20] = "0:/Bank";
        char sbuf[50];
        char file_cnt = nIn;
        printf("@Get form ready into running message\n");

        // read a random file in Bank?/In/?
        // @get string:: 0:/Bank#/In
        sprintf(sbuf, "%d", sBANK + 1);
        strcat(sdir, sbuf);
        strcat(sdir, "/In");
        CRITICAL_FUNC(fres = f_opendir(&fdir, (const TCHAR*)sdir));
        if (fres != FR_OK) {
          printf("Open DIR:%s error:%d\n", sdir, fres);
          break;
        }

        CRITICAL_FUNC(while (file_cnt--) {
          fres = f_readdir(&fdir, &finfo);
          if (fres != FR_OK || finfo.fname[0] == '\0') {
            file_cnt += 1;
            break;
          }
        } fres = f_closedir(&fdir));
        file_cnt = nIn - file_cnt;
        srand(SysTick->VAL);
        file_cnt = rand() % file_cnt;
        file_cnt += 1;

        CRITICAL_FUNC(fres = f_opendir(&fdir, (const TCHAR*)sdir););

        if (fres != FR_OK) {
          printf("Open DIR:%s error:%d\n", sdir, fres);
          break;
        }
        CRITICAL_FUNC(while (file_cnt--) {
          fres = f_readdir(&fdir, &finfo);
        } fres = f_closedir(&fdir));
        // open random file.
        sbuf[0] = '\0';
        strcat(sbuf, sdir);
        strcat(sbuf, "/");
        strcat(sbuf, finfo.fname);
        // get random file path in finfo.
        printf("#Get Random file name:%s\n", sbuf);

        CRITICAL_FUNC(fres = f_open(&file_1, (const TCHAR*)sbuf, FA_READ));
        if (fres != FR_OK) {
          printf("Open file Error:%d\n", fres);
          break;
        }
        NORMAL_READ_PLAY_FILE_1();
        CRITICAL_FUNC(fres = f_close(&file_1));
        printf("a flie closed:%d\n", fres);

        // Loop
        while (loop_flag) {
          // Normal loop option

          // In free time, Search for new cmd
          evt = osMessageGet(SIG_PLAYWAVHandle, 1);
          if (evt.status != osEventMessage) continue;

          switch (evt.value.v) {
            // When System from Running into Ready.
            case SIG_AUDIO_OUTRUN: {
              printf("@Get from running into ready message\n");

              // var init
              sdir[0] = 0;
              sbuf[0] = 0;
              strcat(sdir, "0:/Bank");
              file_cnt = nOut;
              // read a random file in Bank?/Out/?
              // @get string:: 0:/Bank#/Out
              sprintf(sbuf, "%d", sBANK + 1);
              strcat(sdir, sbuf);
              strcat(sdir, "/Out");
              CRITICAL_FUNC(fres = f_opendir(&fdir, (const TCHAR*)sdir));
              if (fres != FR_OK) {
                printf("Open DIR:%s error:%d\n", sdir, fres);
                break;
              }

              CRITICAL_FUNC(while (file_cnt--) {
              fres = f_readdir(&fdir, &finfo);
              if (fres != FR_OK || finfo.fname[0] == '\0') {
              file_cnt += 1;
              break;
              }
              } fres = f_closedir(&fdir));

              file_cnt = nOut - file_cnt;
              srand(SysTick->VAL);
              file_cnt = rand() % file_cnt;
              file_cnt += 1;

              CRITICAL_FUNC(fres = f_opendir(&fdir, (const TCHAR*)sdir););

              if (fres != FR_OK) {
                printf("Open DIR:%s error:%d\n", sdir, fres);
                break;
              }
              CRITICAL_FUNC(while (file_cnt--) {
              fres = f_readdir(&fdir, &finfo);
              } fres = f_closedir(&fdir));
              // open random file.
              sbuf[0] = '\0';
              strcat(sbuf, sdir);
              strcat(sbuf, "/");
              strcat(sbuf, finfo.fname);
              // get random file path in finfo.
              printf("#Get Random file name:%s\n", sbuf);
              CRITICAL_FUNC(fres =
                                f_open(&file_1, (const TCHAR*)sbuf, FA_READ));
              if (fres != FR_OK) {
                printf("Open file Error:%d\n", fres);
                break;
              }
              NORMAL_READ_PLAY_FILE_1();
              CRITICAL_FUNC(fres = f_close(&file_1));
              printf("a flie closed:%d\n", fres);
              loop_flag = 0;
            } break;

            case SIG_AUDIO_TRIGGERB: {
              printf("@Get Triiger B\n");
            } break;

            case SIG_AUDIO_TRIGGERC: {
              printf("@Get Triiger C\n");
            } break;

            case SIG_AUDIO_TRIGGERD: {
              printf("@Get Trigger D\n");
            } break;

            case SIG_AUDIO_TRIGGERE: {
              printf("@Get Trigger E\n");
            } break;

            case SIG_AUDIO_TRIGGEREOFF: {
              printf("@Get Trigger E off\n");
            } break;

            // case ...
            default:
              break;
          }
        }
      } break;

      /* When in ready switching Bank */
      case SIG_AUDIO_BANKSWITCH: {
        char path[30];
        extern __IO uint8_t sBANK;
        printf("@Get switch Bank message\n");
        sprintf(path, "0:/Bank%d", sBANK + 1);
        strcat(path, "/Bankswitch.wav");
        CRITICAL_FUNC(fres = f_open(&file_1, path, FA_READ));
        if (fres != FR_OK) {
          printf("Open file$:%s Error:%d\n", path, fres);
          break;
        }
        NORMAL_READ_PLAY_FILE_1();

        CRITICAL_FUNC(fres = f_close(&file_1));
        printf("a file closed:%d\n", fres);
      } break;
      // case ...
      default:
        break;
    }
  }
}

/* Waiting for Message Queue.
   If no more Message Queue, Stop DAC output
*/
void DACHandle(void const* argument) {
  osEvent evt;
  static enum { running, stopped } flag = stopped;
  for (;;) {
    evt = osMessageGet(pWAVHandle, 59);
    if (evt.status == osEventTimeout) {
      if (flag == stopped) continue;
      printf("Stop DMA\n");
      flag = stopped;
      HAL_GPIO_WritePin(Audio_EN_GPIO_Port, Audio_EN_Pin, GPIO_PIN_RESET);
      continue;
    }

    else if (evt.status == osEventMessage) {
      // Waiting for DAC DMA ConvCpltCall
      if (flag == running) {
        HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)evt.value.p,
                          osFIFO_SIZE, DAC_ALIGN_12B_R);
        osSemaphoreWait(DMA_FLAGHandle, osWaitForever);
      }
      // restart DAC DMA mode
      else {
        printf("Start DMA\n");
        flag = running;
        HAL_TIM_Base_Start(&htim2);
        HAL_GPIO_WritePin(Audio_EN_GPIO_Port, Audio_EN_Pin, GPIO_PIN_SET);
        HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)evt.value.p,
                          osFIFO_SIZE, DAC_ALIGN_12B_R);
        osSemaphoreWait(DMA_FLAGHandle, osWaitForever);
      }
    }
  }
}
void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hdac) {
  static uint32_t CNT;
  osSemaphoreRelease(DMA_FLAGHandle);
  // printf("DMA call back hit:%d\n", ++CNT);
}
