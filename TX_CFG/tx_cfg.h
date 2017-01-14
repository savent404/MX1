#ifndef _TX_CFG_H_
#define _TX_CFG_H_
#include <stdlib.h>
#include <string.h>

#define DEFAULT_TEXT_PATH "0:/SETTING.txt"
#define VAR_NUM 64
#define SBUF_SIZE 200
#define NAME_SIZE 20
#define VAL_SIZE 20
// Default name string

#if LINUX_TEST
#include "stdint.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#else
#include "stm32f1xx_hal.h"
#include "ff.h"
#include "led.h"
#endif
extern const char name_string[][10];
struct config {
  uint32_t Vol;   //音量设置(0无声，1小声，2中等，3大声)
  uint32_t Bank;  //设定TF卡根目录下Bank文件夹数量
                  ///若此值大于Color profiles中的bank计数或者实际文件夹数，则不工作
                  ///若Color profiles中的bank计数或者实际文件夹数大于此设定值，
  //则工作时将多余部分舍弃，只在此值限定范围内循环
  ///范围 1~16
  uint32_t Tpon;   ///关机转待机，Main按钮延时触发值，单位ms
  uint32_t Tpoff;  //待机转关机，Main按钮延时触发值，单位ms
  uint32_t Tout;   //待机转运行，Main按钮延时触发值，单位ms
  uint32_t Tin;    //运行转待机，Main按钮延时触发值，单位ms
  //切换LED色彩配置以及音频的AUX按钮延时触发值，单位ms
  uint32_t Ts_switch;
  //运行状态下，加速度计以及按钮连续无触发计时,
  //到达则自动转待机，0表示不自动转待机，单位ms
  uint32_t Tautoin;
  //待机状态下，按钮连续无动作计时触发关机，0表示不自动关机，单位ms
  uint32_t Tautooff;
  uint32_t Tmute;  //关机状态下，进入静音模式的双键复合触发时长
  uint32_t TLcolor;  //开机状态下，LEDbank变更的双键复合触发时长
                     //若TLcolor>TEtrigger,或TLcolor>Tin,则此参数无效
  uint32_t TBfreeze;  //触发B重复触发间隔，单位ms
  uint32_t TCfreeze;  //触发C重复触发间隔，单位ms
  uint32_t TCflip;  //触发C的LED色彩由Bank翻转至Fbank的单次时长，单位ms
  uint32_t Ccount;  //触发C的单次触发色彩翻转次数，翻转间隔为TCflip

  uint32_t TDfreeze;  //触发D重复触发间隔，单位ms
  uint32_t TDflip;  //触发D的LED色彩由Bank翻转至Fbank的时长，单位ms
  uint32_t Dcount;  //触发D的单次触发色彩翻转次数,翻转间隔为TDflip

  uint32_t TEtrigger;  //开机状态下，触发E的AUX按钮触发时长，单位ms
  uint32_t Ecycle;   //触发E的Bank色与Fbank色交替周期，单位ms
  uint32_t TLon;     // LED从起辉到达到设定全局亮度的时间
  uint32_t TLoff;    // LED从全局亮度到熄灭的时间
  uint32_t Lbright;  // LED全局亮度，范围0~1023
  uint32_t Lmode;    //开机运行时LED基础工作模式
                     // 0=一直常亮
  // 1=0.1s为单位，LED全局亮度在设定值的70~100%间随机变化
  /// 2=3s为周期，LED全局呼吸灯，亮度在设定值的70~100%间正弦变化
  //此模式仅影响常态运行，触发均不受其限制
  uint32_t memsHz;   //加速度计设定扫描频率
  uint32_t memsA;    //加速度计最小采样精度
  uint32_t memsThr;  //加速度计阈值
  uint32_t memsGap;  //加速度计采样间隔

  uint32_t S1;  //触发B，加速度计下限值，范围0~1023
  uint32_t Sh;  //触发B，加速度计上限值，范围0~1023
  uint32_t Cl;  //触发C，加速度计下限值，范围0~1023
  uint32_t Ch;  //触发C, 加速度计上限值，范围0~1023
                /* 因未接触过lis3dh相关开发，
                加速度计部分以开发实际使用设参为准，范围选择0~2G即可
                */

  /*
   .....
   */
};
void TX_CFG(struct config *cfg, RGBL rgbl[][2]);
/**
  * @Brief   Get ans:val string
  * @Para line src-string
  * @Para name des-name-string
  * @Para val  des-val-string
  */
static void TextNameVal(char *line, char *name, char *val);

#if LINUX_TEST

/**
  * @Brief  Get a line from pt
  * @Para   des output to des
  * @Retval should sub number
  */
static int TextLine(int fd, char *des);

#else
static size_t TextLine(FIL *pt, char *des);
#endif

#endif
