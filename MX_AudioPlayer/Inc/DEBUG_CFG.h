#ifndef _DEBUG_CFG_H_
#define _DEBUG_CFG_H_

#include <stdio.h>

#ifndef DEBUG_FATFS
#define DEBUG_FATFS 0
#endif
#if DEBUG_FATFS
#define printf_FATFS printf
#else
#define printf_FATFS(x,...) ;
#endif //DEBUG_FATFS


#ifndef DEBUG_DACDMA
#define DEBUG_DACDMA 0
#endif
#if DEBUG_DACDMA
#define printf_DACDMA printf
#else
#define printf_DACDMA(x,...) ;
#endif //DEBUG_DACDMA


#ifndef DEBUG_AUDIOMESSAGE
#define DEBUG_AUDIOMESSAGE 0
#endif
#if DEBUG_AUDIOMESSAGE
#define printf_AUDIOMESSAGE printf
#else
#define printf_AUDIOMESSAGE(x,...) ;
#endif //DEBUG_AUDIOMESSAGE


#ifndef DEBUG_TRIGGERFREEZ
#define DEBUG_TRIGGERFREEZ 0
#endif
#if DEBUG_TRIGGERFREEZ
#define printf_TRIGGERFREEZ printf
#else
#define printf_TRIGGERFREEZ(x,...) ;
#endif //DEBUG_TRIGGERFREEZ


#ifndef DEBUG_RANDOMFILE
#define DEBUG_RANDOMFILE 0
#endif
#if DEBUG_RANDOMFILE
#define printf_RANDOMFILE printf
#else
#define printf_RANDOMFILE(x,...) ;
#endif //DEBUG_RANDOMFILE

#ifndef DEBUG_KEY
#define DEBUG_KEY 0
#endif
#if DEBUG_KEY
#define printf_KEY printf
#else
#define printf_KEY(x,...) ;
#endif //DEBUG_KEY

#ifndef DEBUG_SYSTEM
#define DEBUG_SYSTEM 1
#endif
#if DEBUG_SYSTEM
#define printf_SYSTEM printf
#else
#define printf_SYSTEM(x,...) ;
#endif //DEBUG_SYSTEM


#ifndef DEBUG_LED
#define DEBUG_LED 0
#endif
#if DEBUG_LED
#define printf_LED printf
#else 
#define printf_LED(x,...) ;
#endif
#endif //DEBUG_LED
