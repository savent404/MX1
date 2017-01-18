#ifndef _TX_CFG_H_
#define _TX_CFG_H_

#include <stdlib.h>
#include <string.h>

#define DEFAULT_TEXT_PATH "0://SETTING.txt"
#define SBUF_SIZE 200
#define NAME_SIZE 20

#include "stm32f1xx_hal.h"
#include "ff.h"
#include "led.h"

struct config {
  uint32_t Vol;  //音量设置(0无声，1小声，2中等，3大声)

  uint32_t Tpon;   //关机转待机，Main按钮延时触发值，单位ms,
  uint32_t Tpoff;  //待机转关机，Main按钮延时触发值，单位ms,
  uint32_t Tout;   //待机转运行，Main按钮延时触发值，单位ms,
  uint32_t Tin;    //运行转待机，Main按钮延时触发值，单位ms,
  uint32_t Ts_switch;  //切换LED色彩配置以及音频的AUX按钮延时触发值，单位ms,

  uint32_t
      Tautoin;  //运行状态下，加速度计以及按钮连续无触发计时，到达则自动转待机，0表示不自动转待机，单位ms,
  uint32_t
      Tautooff;  //待机状态下，按钮连续无动作计时触发关机，0表示不自动关机，单位ms,

  uint32_t Tmute;  //关机状态下，进入静音模式的双键复合触发时长
  uint32_t TLcolor;  //开机状态下，LEDbank变更的双键复合触发时长
                     //若TLcolor>TEtrigger,或TLcolor>Tin,则此参数无效

  uint32_t TBfreeze;  //触发B重复触发间隔，单位ms
  uint32_t TBMode;    // TriggerLMode,可复选

  uint32_t TCfreeze;  //触发C重复触发间隔，单位ms
  uint32_t TCMode;  // TriggerLMode,可复选，单次触发只随机执行一种

  uint32_t TDfreeze;  //触发D重复触发间隔，单位ms
  uint32_t TDMode;  // TriggerLMode,可复选，单次触发只随机执行一种

  uint32_t TEtrigger;  //开机状态下，触发E的AUX按钮触发时长，单位ms
  uint32_t TEMode;  // TriggerLMode,可复选

  uint32_t TLon;  // LED从起辉到达到设定全局亮度的时间，单位ms
  uint32_t TLoff;    // LED从全局亮度到熄灭的时间，单位ms
  uint32_t Lbright;  // LED全局亮度，范围0~1023
  uint32_t Ldeep;  // LED全局亮度下探值，范围0~1023，当LDeep>Lbright时，此值归0
  uint32_t LMode;  //开机运行时LED基础工作模式，详见Config

  uint32_t memsHz;   //加速度计设定扫描频率
  uint32_t memsA;    //加速度计最小采样精度
  uint32_t memsThr;  //加速度计阈值
  uint32_t memsGap;  //加速度计采样间隔

  uint32_t Sl;  //触发B，加速度计下限值，范围0~1023
  uint32_t Sh;  //触发B，加速度计上限值，范围0~1023
  uint32_t Cl;  //触发C，加速度计下限值，范围0~1023
  uint32_t Ch;  //触发C, 加速度计上限值，范围0~1023
};

void TX_CFG(struct config *cfg, RGBL rgbl[][2]);

#endif
