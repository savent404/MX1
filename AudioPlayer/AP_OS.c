/*
   AP_OS

   文件説明
   處理有關于音頻播放的任務

   系統函數
   @WAVHandle 處理系統發送的[音頻播放請求]：
   *打開需要播放的音頻文件
   *將16位PCM數據轉換為STM32DAC需要的12位數據
   *將需要輸出的DAC數據數組的首地址通過隊列的方式發送給處理音頻數據輸出的函數@DACHandle
   @DACHandle 處理系統發送的[音頻數據]:
   *從隊列中獲取需要輸出的數組地址（數組大小固定，無需傳送數據量大小）
   *若暫時沒有數據到來，關閉D類功放使能脚以減小輸出噪音的可能
   *若有數據到來，將拉低的D類功放使能脚拉高，並使用DMA方式輸出DAC數據
   *等待DMA操作完成(通過檢測[Semaphore] @DMA_FLAGHandle 二值信號量)

   私有函數
   @convert_single 將一個16位PCM數據轉化爲DAC輸出格式
   @convert_double 將兩個16位PCM數據轉化爲DAC輸出格式
   @NORMAL_READ_PLAY_FILE_1 宏定義函數，代表讀取一個PCM文件的標準操作
   為減少堆棧溢出的可能，故使用宏定義的方式
   @MIX_READ_PLAY_FILE_12   宏定義函數，代表讀取兩個PCM文件的標準操作
   為減少堆棧溢出的可能，故使用宏定義的方式

   私有變量
   #SIG_AUDIO???         為@WAVHandle 可以接收的指令
   #audio_buf1&2         輸出數據的數組
   #fres                 FATFS 函數返回信息
   #fdir                 FATFS 路徑變量
   #finfo                FATFS 文件變量
   #osFIFO_SIZE          輸出數據的最小單位，即每一次DMA輸出的最小單位
   #osFIFO_NUM           由freeRTOS中[osMessageQId]@pWAVHandle 定義的大小決定
                         @pWAVHandle 隊列大小+2 = osFIFO_NUM

   其他説明
   通過printf方式輸出調試信息，工程中通過ITM swo脚輸出
   若需要屏蔽所有調試信息，可嘗試
   #define printf(x,...) ;
*/
#include "AP_OS.h"
#include "fatfs.h"
#include "string.h"
#include "USR_CFG.h"
#include "stdlib.h"
#include "DEBUG_CFG.h"
#include <stdint.h>
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
/* When LED change bank */
const uint8_t SIG_AUDIO_COLORSWITCH = 0x0A;
/* Warnning Low Power */
const uint8_t SIG_AUDIO_LOWPOWER = 0x0B;
/* Warnning restart */
const uint8_t SIG_AUDIO_RESTART = 0x0C;

// wave pcm buffer, tow channels
static uint16_t audio_buf1[osFIFO_NUM][osFIFO_SIZE],
    audio_buf2[osFIFO_NUM][osFIFO_SIZE];
static FRESULT fres;
static DIR fdir;
static FILINFO finfo;

__inline __weak uint16_t convert_single(uint16_t src) {
  return ((src + 0x7FFF) >> 4);
}
__inline __weak uint16_t convert_double(int16_t src_1, int16_t src_2) {
  /*  src_1 += 0x7FFF;
    src_2 += 0x7FFF;
    src_1 >>= 4;
    src_2 >>= 4;
    return (src_1 / 2 + src_2);*/
  static float f = 1;
  int32_t buf = src_1 + src_2;
  if (buf > INT16_MAX / 2) {
    f = (float)INT16_MAX / 2 / (float)buf;
    buf = INT16_MAX / 2;
  }
  buf *= f;
  if (buf < INT16_MIN / 2) {
    f = (float)INT16_MIN / 2 / (float)buf;
    buf = INT16_MIN / 2;
  }
  if (f < 1) {
    f += ((float)1 - f) / (float)32;
  }
  return (buf >> 4) + 0x1000 / 2;
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
#define NORMAL_READ_PLAY_FILE_1()                                           \
  taskENTER_CRITICAL();                                                     \
  fres = f_read(&file_1, &pcm1, sizeof(pcm1), &cnt_1);                      \
  fres = f_read(&file_1, &data1, sizeof(data1), &cnt_1);                    \
  taskEXIT_CRITICAL();                                                      \
  while (1) {                                                               \
    /* No enuough buffer, exit */                                           \
    if (data1.size < osFIFO_SIZE) break;                                    \
                                                                            \
    if (fres != FR_OK) {                                                    \
      printf_FATFS("FATFS:Something wrong with FATFS. code:%d\n", fres);    \
      f_close(&file_1);                                                     \
      break;                                                                \
    }                                                                       \
    /* Read buffer and waiting for put succese. */                          \
    taskENTER_CRITICAL();                                                   \
    fres = f_read(&file_1, audio_buf1[pos1], osFIFO_SIZE * sizeof(int16_t), \
                  &cnt_1);                                                  \
    taskEXIT_CRITICAL();                                                    \
    fpt_1 = audio_buf1[pos1];                                               \
    for (i = 0; i < osFIFO_SIZE; i++) {                                     \
      *fpt_1 = convert_single(*fpt_1);                                      \
      fpt_1++;                                                              \
    }                                                                       \
    data1.size -= cnt_1;                                                    \
    osstatus = osMessagePut(pWAVHandle, (uint32_t)audio_buf1[pos1], 0);     \
    while (osstatus) {                                                      \
      osstatus = osMessagePut(pWAVHandle, (uint32_t)audio_buf1[pos1], 0);   \
    }                                                                       \
    pos1 += 1;                                                              \
    pos1 %= osFIFO_NUM;                                                     \
  }
/* A normal option to read two file then put to DACHandle
   If needs to play two PCM file, then use it.

   @Note: using CRITICAL func to protect SDIO's opt would
          not to be interrupt.
*/
#define MIX_READ_PLAY_FILE_12()                                              \
  CRITICAL_FUNC(fres = f_read(&file_1, &pcm1, sizeof(pcm1), &cnt_1);         \
                fres = f_read(&file_1, &data1, sizeof(data1), &cnt_1));      \
                                                                             \
  /*Mix Player*/                                                             \
  while (1) {                                                                \
    /* firstly read file 1:Trigger file*/                                    \
    if (data1.size < osFIFO_SIZE) break;                                     \
    if (fres != FR_OK) {                                                     \
      printf_FATFS("FATFS:Something wrong wit FATFS. code:%d\n", fres);      \
      f_close(&file_1);                                                      \
      break;                                                                 \
    }                                                                        \
    CRITICAL_FUNC(fres = f_read(&file_1, audio_buf1[pos1],                   \
                                osFIFO_SIZE * sizeof(int16_t), &cnt_1));     \
    if (fres != FR_OK) {                                                     \
      printf_FATFS("FATFS:Something wrong with FATFS. code:%d\n", fres);     \
      f_close(&file_1);                                                      \
      break;                                                                 \
    }                                                                        \
    /*Then read File 2:hum.wav */                                            \
    if (data2.size < osFIFO_SIZE) {                                          \
      f_lseek(&file_2, 0);                                                   \
      CRITICAL_FUNC(fres = f_read(&file_2, &pcm2, sizeof(pcm2), &cnt_2);     \
                    fres = f_read(&file_2, &data2, sizeof(data2), &cnt_2);); \
      pos2 = 0;                                                              \
    } /* if file is end, reopen it.*/                                        \
    CRITICAL_FUNC(fres = f_read(&file_2, audio_buf2[pos2],                   \
                                osFIFO_SIZE * sizeof(int16_t), &cnt_2));     \
    if (fres != FR_OK) {                                                     \
      printf_FATFS("FATFS:Something wrong with FATFS. code:%d\n", fres);     \
    }                                                                        \
    fpt_1 = audio_buf1[pos1];                                                \
    fpt_2 = audio_buf2[pos2];                                                \
    for (i = 0; i < osFIFO_SIZE; i++) {                                      \
      *fpt_1 = convert_double(*fpt_1, *fpt_2);                               \
      fpt_1++, fpt_2++;                                                      \
    }                                                                        \
    data1.size -= cnt_1;                                                     \
    data2.size -= cnt_2;                                                     \
    osstatus = osMessagePut(pWAVHandle, (uint32_t)audio_buf1[pos1], 0);      \
    while (osstatus) {                                                       \
      osstatus = osMessagePut(pWAVHandle, (uint32_t)audio_buf1[pos1], 0);    \
    }                                                                        \
    pos1 += 1;                                                               \
    pos1 %= osFIFO_NUM;                                                      \
    pos2 += 1;                                                               \
    pos2 %= osFIFO_NUM;                                                      \
  }
/*  Waiting for CMD form another Handle
    * Play single Wav file
    * Play double Wav file
*/
void WAVHandle(void const* argument) {
  uint8_t pos1 = 0, pos2 = 0;
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
        printf_RANDOMFILE("#Get startup message\n");
        CRITICAL_FUNC(fres = f_open(&file_1, "0:/System/Boot.wav", FA_READ));
        if (fres != FR_OK) {
          printf_FATFS("FATFS:Open file:%s Error:%d\n", "0:/System/Boot.wav",
                       fres);
          break;
        }

        NORMAL_READ_PLAY_FILE_1();

        CRITICAL_FUNC(fres = f_close(&file_1));

        printf_FATFS("FATFS:a flie:[%s] closed:%d\n", "0:/System/Boot.wav",
                     fres);
      } break;

      // When System from ready into close
      case SIG_AUDIO_POWEROFF: {
        printf_RANDOMFILE("#Get Power off message\n");
        CRITICAL_FUNC(fres =
                          f_open(&file_1, "0:/System/Poweroff.wav", FA_READ));
        if (fres != FR_OK) {
          printf_FATFS("FATFS:Open file:%s Error:%d\n",
                       "0:/System/Poweroff.wav", fres);
          break;
        }

        NORMAL_READ_PLAY_FILE_1();

        CRITICAL_FUNC(fres = f_close(&file_1));

        printf_FATFS("FATFS:a flie:[%s] closed:%d\n", "0:/System/Poweroff.wav",
                     fres);
      } break;

      // When System from Ready into running.
      case SIG_AUDIO_INTORUN: {
        // Init Option
        uint8_t loop_flag = 1;
        extern __IO uint8_t sBANK;
        char sdir[20] = "0:/Bank";
        char sbuf[50];
        char file_cnt = nIn;

        // Init function
        {
          printf_RANDOMFILE("#Get form ready into running message\n");
          // read a random file in Bank?/In/?
          // @get string:: 0:/Bank#/In
          sprintf(sbuf, "%d/In", sBANK + 1);
          strcat(sdir, sbuf);
          CRITICAL_FUNC(fres = f_opendir(&fdir, (const TCHAR*)sdir));
          if (fres != FR_OK) {
            printf_FATFS("FATFS:Open DIR:%s error:%d\n", sdir, fres);
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
            printf_FATFS("FATFS:Open DIR:%s error:%d\n", sdir, fres);
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
          printf_RANDOMFILE("#Get Random file name:%s\n", sbuf);
          CRITICAL_FUNC(fres = f_open(&file_1, (const TCHAR*)sbuf, FA_READ));
          if (fres != FR_OK) {
            printf_FATFS("FATFS:Open file:%s Error:%d\n", sbuf, fres);
            break;
          }
          NORMAL_READ_PLAY_FILE_1();
          CRITICAL_FUNC(fres = f_close(&file_1));
          printf_FATFS("FATFS:a flie:[%s] closed:%d\n", sbuf, fres);
        }

        // Open hum.wav
        {
          sprintf(sbuf, "0:/Bank%d/hum.wav", sBANK + 1);
          CRITICAL_FUNC(fres = f_open(&file_2, sbuf, FA_READ));
          if (fres != FR_OK) {
            printf_FATFS("FATFS:Open file:%s Error:%d\n", sbuf, fres);
            break;
          }
          CRITICAL_FUNC(fres = f_read(&file_2, &pcm2, sizeof(pcm2), &cnt_2);
                        fres = f_read(&file_2, &data2, sizeof(data2), &cnt_2));
        }
        // Loop
        while (loop_flag) {
          // Normal loop option, play hum.wav

          // In free time, Search for new cmd
          evt = osMessageGet(SIG_PLAYWAVHandle, 1);

          // No message come, play hum.wav file
          if (evt.status != osEventMessage) {
            // read a buffer first, or reopen hum.wav and read
            if (data2.size < osFIFO_SIZE) {
              f_lseek(&file_2, 0);
              CRITICAL_FUNC(
                  fres = f_read(&file_2, &pcm2, sizeof(pcm2), &cnt_2);
                  fres = f_read(&file_2, &data2, sizeof(data2), &cnt_2););
            }
            CRITICAL_FUNC(fres = f_read(&file_2, audio_buf2[pos2],
                                        osFIFO_SIZE * sizeof(int16_t), &cnt_2));
            data2.size -= cnt_2;
            if (fres != FR_OK) {
              printf_FATFS("Something wrong with FATFS. code:%d\n", fres);
            }
            fpt_2 = audio_buf2[pos2];
            for (i = 0; i < osFIFO_SIZE; i++) {
              *fpt_2 = convert_single(*fpt_2);
              fpt_2++;
            }
            osstatus = osMessagePut(pWAVHandle, (uint32_t)audio_buf2[pos2], 0);
            while (osstatus) {
              osstatus =
                  osMessagePut(pWAVHandle, (uint32_t)audio_buf2[pos2], 0);
            }
            pos2 += 1;
            pos2 %= osFIFO_NUM;
            continue;
          }

          switch (evt.value.v) {
            // When System from Running into Ready.
            case SIG_AUDIO_OUTRUN: {
              printf_RANDOMFILE("#Get from running into ready message\n");

              // close hum.wav
              CRITICAL_FUNC(fres = f_close(&file_2));
              if (fres != FR_OK) {
                printf_FATFS("FATFS:a file:[0:/Bank%d/hum.wav] closed:%d",
                             sBANK + 1, fres);
              }

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
                printf_FATFS("Open DIR:[%s] error:%d\n", sdir, fres);
                break;
              }
              CRITICAL_FUNC(while (file_cnt--) {
                CRITICAL_FUNC(fres = f_readdir(&fdir, &finfo););
                if (fres != FR_OK || finfo.fname[0] == '\0') {
                  file_cnt += 1;
                  break;
                }
              } CRITICAL_FUNC(fres = f_closedir(&fdir)));
              file_cnt = nOut - file_cnt;
              srand(SysTick->VAL);
              file_cnt = rand() % file_cnt;
              file_cnt += 1;

              CRITICAL_FUNC(fres = f_opendir(&fdir, (const TCHAR*)sdir););

              if (fres != FR_OK) {
                printf_FATFS("Open DIR:[%s] error:%d\n", sdir, fres);
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
              printf_RANDOMFILE("#Get Random file name:%s\n", sbuf);
              CRITICAL_FUNC(fres =
                                f_open(&file_1, (const TCHAR*)sbuf, FA_READ));
              if (fres != FR_OK) {
                printf_FATFS("Open file Error:%d\n", fres);
                break;
              }
              NORMAL_READ_PLAY_FILE_1();
              CRITICAL_FUNC(fres = f_close(&file_1));
              printf_FATFS("FATFS:a file:[%s] closed:%d\n", sbuf, fres);
              loop_flag = 0;
            } break;

            case SIG_AUDIO_TRIGGERB: {
              printf_RANDOMFILE("#Get Triiger B\n");
              // get random trigger B file
              {
                sprintf(sdir, "0:/Bank%d/Trigger_B", sBANK + 1);
                CRITICAL_FUNC(fres = f_opendir(&fdir, (const TCHAR*)sdir));
                if (fres != FR_OK) {
                  printf_FATFS("Open DIR:[%s] error:%d\n", sdir, fres);
                  break;
                }
                file_cnt = nTrigger_D;
                CRITICAL_FUNC(while (file_cnt--) {
                  fres = f_readdir(&fdir, &finfo);
                  if (fres != FR_OK || finfo.fname[0] == '\0') {
                    file_cnt += 1;
                    break;
                  }
                } fres = f_closedir(&fdir));
                file_cnt = nTrigger_D - file_cnt;
                srand(SysTick->VAL);
                file_cnt = rand() % file_cnt;
                file_cnt += 1;
                CRITICAL_FUNC(fres = f_opendir(&fdir, (const TCHAR*)sdir););
                if (fres != FR_OK) {
                  printf_FATFS("Open DIR:[%s] error:%d\n", sdir, fres);
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
                printf_RANDOMFILE("#Get Random file name:%s\n", sbuf);
                CRITICAL_FUNC(fres =
                                  f_open(&file_1, (const TCHAR*)sbuf, FA_READ));
                if (fres != FR_OK) {
                  printf_FATFS("Open file Error:%d\n", fres);
                  break;
                }
              }
              MIX_READ_PLAY_FILE_12();
              CRITICAL_FUNC(fres = f_close(&file_1));
              printf_FATFS("FATFS:a flie:[%s] closed:%d\n", sbuf, fres);
            } break;

            case SIG_AUDIO_TRIGGERC: {
              printf_RANDOMFILE("#Get Triiger C\n");
              // get random trigger C file
              {
                sprintf(sdir, "0:/Bank%d/Trigger_C", sBANK + 1);
                CRITICAL_FUNC(fres = f_opendir(&fdir, (const TCHAR*)sdir));
                if (fres != FR_OK) {
                  printf_FATFS("Open DIR:[%s] error:%d\n", sdir, fres);
                  break;
                }
                file_cnt = nTrigger_D;
                CRITICAL_FUNC(while (file_cnt--) {
                  fres = f_readdir(&fdir, &finfo);
                  if (fres != FR_OK || finfo.fname[0] == '\0') {
                    file_cnt += 1;
                    break;
                  }
                } fres = f_closedir(&fdir));
                file_cnt = nTrigger_D - file_cnt;
                srand(SysTick->VAL);
                file_cnt = rand() % file_cnt;
                file_cnt += 1;
                CRITICAL_FUNC(fres = f_opendir(&fdir, (const TCHAR*)sdir););
                if (fres != FR_OK) {
                  printf_FATFS("Open DIR:[%s] error:%d\n", sdir, fres);
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
                printf_RANDOMFILE("#Get Random file name:%s\n", sbuf);
                CRITICAL_FUNC(fres =
                                  f_open(&file_1, (const TCHAR*)sbuf, FA_READ));
                if (fres != FR_OK) {
                  printf_FATFS("Open file Error:%d\n", fres);
                  break;
                }
              }
              MIX_READ_PLAY_FILE_12();
              CRITICAL_FUNC(fres = f_close(&file_1));
              printf_FATFS("FATFS:a file:[%s] closed:%d\n", sbuf, fres);
            } break;

            case SIG_AUDIO_TRIGGERD: {
              printf_RANDOMFILE("#Get Trigger D\n");
              // get random trigger D file
              {
                sprintf(sdir, "0:/Bank%d/Trigger_D", sBANK + 1);
                CRITICAL_FUNC(fres = f_opendir(&fdir, (const TCHAR*)sdir));
                if (fres != FR_OK) {
                  printf_FATFS("Open DIR:[%s] error:%d\n", sdir, fres);
                  break;
                }
                file_cnt = nTrigger_D;
                CRITICAL_FUNC(while (file_cnt--) {
                  fres = f_readdir(&fdir, &finfo);
                  if (fres != FR_OK || finfo.fname[0] == '\0') {
                    file_cnt += 1;
                    break;
                  }
                } fres = f_closedir(&fdir));
                file_cnt = nTrigger_D - file_cnt;
                srand(SysTick->VAL);
                file_cnt = rand() % file_cnt;
                file_cnt += 1;
                CRITICAL_FUNC(fres = f_opendir(&fdir, (const TCHAR*)sdir););
                if (fres != FR_OK) {
                  printf_FATFS("Open DIR:[%s] error:%d\n", sdir, fres);
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
                printf_RANDOMFILE("#Get Random file name:%s\n", sbuf);
                CRITICAL_FUNC(fres =
                                  f_open(&file_1, (const TCHAR*)sbuf, FA_READ));
                if (fres != FR_OK) {
                  printf_FATFS("Open file Error:%d\n", fres);
                  break;
                }
              }
              MIX_READ_PLAY_FILE_12();
              CRITICAL_FUNC(fres = f_close(&file_1));
              printf_FATFS("a flie closed:%d\n", fres);
            } break;

            case SIG_AUDIO_TRIGGERE: {
              printf_RANDOMFILE("#Get Trigger E\n");
              // read a random file in Bank?/Trigger_E/?
              // @Get string:: 0:/Bank#/Trigger_E
              {
                sprintf(sdir, "0:/Bank%d/Trigger_E", sBANK + 1);
                CRITICAL_FUNC(fres = f_opendir(&fdir, sdir));
                if (fres != FR_OK) {
                  printf_FATFS("Open DIR:[%s] error:%d\n", sdir, fres);
                  break;
                }
                file_cnt = nTrigger_E;
                CRITICAL_FUNC(while (file_cnt--) {
                  fres = f_readdir(&fdir, &finfo);
                  if (fres != FR_OK || finfo.fname[0] == '\0') {
                    file_cnt += 1;
                    break;
                  }
                } fres = f_closedir(&fdir));
                file_cnt = nTrigger_E - file_cnt;
                srand(SysTick->VAL);
                file_cnt = rand() % file_cnt;
                file_cnt += 1;
                CRITICAL_FUNC(fres = f_opendir(&fdir, sdir));
                if (fres != FR_OK) {
                  printf_FATFS("Open DIR:[%s] error:%d\n", sdir, fres);
                  break;
                }
                CRITICAL_FUNC(while (file_cnt--) {
                  fres = f_readdir(&fdir, &finfo);
                } fres = f_closedir(&fdir));
                // Open reandom file.
                sbuf[0] = '\0';
                strcat(sbuf, sdir);
                strcat(sbuf, "/");
                strcat(sbuf, finfo.fname);
                // Get reandom file path in finfo.
                printf_RANDOMFILE("#Get Random file name:%s\n", sbuf);
              }
              // Loop's Init
              CRITICAL_FUNC(fres = f_open(&file_1, sbuf, FA_READ));
              if (fres != FR_OK) {
                printf_FATFS("FATFS:Open file:%s Error:%d\n", sbuf, fres);
                break;
              }
              CRITICAL_FUNC(fres = f_read(&file_1, &pcm1, sizeof(pcm1), &cnt_1);
                            fres =
                                f_read(&file_1, &data1, sizeof(data1), &cnt_1);)
              // Loop
              while (1) {
                evt = osMessageGet(SIG_PLAYWAVHandle, 1);
                if (evt.status == osEventMessage &&
                    evt.value.v == SIG_AUDIO_TRIGGEREOFF) {
                  printf_RANDOMFILE("#Get Trigger E off\n");
                  break;
                }
                if (data1.size < osFIFO_SIZE) {
                  CRITICAL_FUNC(
                      fres = f_lseek(&file_1, 0);
                      fres = f_read(&file_1, &pcm1, sizeof(pcm1), &cnt_1);
                      fres = f_read(&file_1, &data1, sizeof(data1), &cnt_1););
                }

                CRITICAL_FUNC(fres = f_read(&file_1, audio_buf1[pos1],
                                            osFIFO_SIZE * sizeof(int16_t),
                                            &cnt_1));
                fpt_1 = audio_buf1[pos1];
                for (i = 0; i < osFIFO_SIZE; i++) {
                  *fpt_1 = convert_single(*fpt_1);
                  fpt_1++;
                }
                data1.size -= cnt_1;
                osstatus =
                    osMessagePut(pWAVHandle, (uint32_t)audio_buf1[pos1], 0);
                while (osstatus) {
                  osstatus =
                      osMessagePut(pWAVHandle, (uint32_t)audio_buf1[pos1], 0);
                }
                pos1 += 1;
                pos1 %= osFIFO_NUM;
              }
              CRITICAL_FUNC(fres = f_close(&file_1));
              printf_FATFS("FATFS:a file:[%s] closed:%d", sbuf, fres);
            } break;

            /*case SIG_AUDIO_TRIGGEREOFF: {
              printf_RANDOMFILE("#Get Trigger E off\n");
            } break;*/
            case SIG_AUDIO_COLORSWITCH: {
              printf_RANDOMFILE("#Get LED switch Color message\n");
              CRITICAL_FUNC(
                  fres = f_open(&file_1, (const TCHAR*)"system/colorswitch.wav",
                                FA_READ));
              if (fres != FR_OK) {
                printf_FATFS("Open file Error:%d\n", fres);
                break;
              }
              MIX_READ_PLAY_FILE_12();
              CRITICAL_FUNC(fres = f_close(&file_1));
              printf_FATFS("FATFS:a file:[%s] closed:%d\n",
                           "system/colorswitch.wav", fres);
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
        printf_RANDOMFILE("#Get switch Bank message\n");
        sprintf(path, "0:/Bank%d", sBANK + 1);
        strcat(path, "/Bankswitch.wav");
        CRITICAL_FUNC(fres = f_open(&file_1, path, FA_READ));
        if (fres != FR_OK) {
          printf_FATFS("FATFS:Open file:%s Error:%d\n", path, fres);
          break;
        }
        NORMAL_READ_PLAY_FILE_1();

        CRITICAL_FUNC(fres = f_close(&file_1));
        printf_FATFS("FATFS:a file:[%s] closed:%d\n", path, fres);
      } break;
      case SIG_AUDIO_LOWPOWER: {
        uint8_t cnt = 2;
        while (cnt--) {
          printf_RANDOMFILE("#Get lowpower message\n");
          CRITICAL_FUNC(fres =
                            f_open(&file_1, "0:/System/lowpower.wav", FA_READ));
          if (fres != FR_OK) {
            printf_FATFS("FATFS:Open file:%s Error:%d\n",
                         "0:/System/lowpower.wav", fres);
            break;
          }

          NORMAL_READ_PLAY_FILE_1();

          CRITICAL_FUNC(fres = f_close(&file_1));

          printf_FATFS("FATFS:a flie:[%s] closed:%d\n",
                       "0:/System/lowpower.wav", fres);
        }
      } break;

      case SIG_AUDIO_RESTART: {
        printf_RANDOMFILE("#Get recharge message\n");
        CRITICAL_FUNC(fres =
                          f_open(&file_1, "0:/System/recharge.wav", FA_READ));
        if (fres != FR_OK) {
          printf_FATFS("FATFS:Open file:%s Error:%d\n",
                       "0:/System/recharge.wav", fres);
          break;
        }

        NORMAL_READ_PLAY_FILE_1();

        CRITICAL_FUNC(fres = f_close(&file_1));

        printf_FATFS("FATFS:a flie:[%s] closed:%d\n", "0:/System/recharge.wav",
                     fres);
        HAL_GPIO_WritePin(Power_EN_GPIO_Port, Power_EN_Pin, GPIO_PIN_RESET);
      }
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
    evt = osMessageGet(pWAVHandle, 5);
    if (evt.status == osEventTimeout) {
      if (flag == stopped) continue;
      printf_DACDMA("Stop DMA\n");
      flag = stopped;
      HAL_GPIO_WritePin(Audio_Soft_EN_GPIO_Port, Audio_Soft_EN_Pin,
                        GPIO_PIN_RESET);
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
        printf_DACDMA("Start DMA\n");
        flag = running;
        HAL_TIM_Base_Start(&htim2);
        HAL_GPIO_WritePin(Audio_EN_GPIO_Port, Audio_EN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(Audio_Soft_EN_GPIO_Port, Audio_Soft_EN_Pin,
                          GPIO_PIN_SET);
        HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)evt.value.p,
                          osFIFO_SIZE, DAC_ALIGN_12B_R);
        osSemaphoreWait(DMA_FLAGHandle, osWaitForever);
      }
    }
  }
}
void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef* hdac) {
  osSemaphoreRelease(DMA_FLAGHandle);
}
