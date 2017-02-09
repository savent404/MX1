#ifndef USR_CFG_H_
#define USR_CFG_H_

#define nBank 2         //设定TF卡根目录下Bank文件夹的最大数量
		                //若此值大于Color profiles中的bank计数或者实际文件夹数，则只计三者中的最小值
		                //ofiles中的bank计数或者实际文件夹数大于此设定值，则工作时将多余部分舍弃，只在此值限定范围内循环
#define nIn 6           //设定Bank#文件夹下In文件夹内允许最大文件数量
#define nOut 2		    //设定Bank#文件夹下Out文件夹内允许最大文件数量
#define nTrigger_B 16   //设定Bank#文件夹下Trigger_B文件夹内允许最大文件数量
#define nTrigger_C 16   //设定Bank#文件夹下Trigger_C文件夹内允许最大文件数量
#define nTrigger_D 8    //设定Bank#文件夹下Trigger_D文件夹内允许最大文件数量
#define nTrigger_E 2    //设定Bank#文件夹下Trigger_E文件夹内允许最大文件数量

////以上配置，可设置范围1~16或1~32,看程序实现而定
/*typedef enum {
    L_STATIC = 1,
    L_BREATHH = 2,
    L_SLOW_PLUSE = 3,
    L_MEDIUM_PLUSE = 4,
    L_FAST_PLUSE = 5
}_LMode;*/
#define _LMode 2
#define T_Breath (2000/2)
#define Lbright_Ldeep 10
/*
##_LMode，常规模式以及可设参项
	  
		  //1=STATIC  静止 保持亮度不变化
		  //2=BREATH  呼吸 以T_Breath为周期，Lbright-Ldeep为下限，亮度近似正弦周期性变化
		  //3=SLOW PLUSE   慢速脉冲  以T_SP为周期，Lbright-Ldeep为下限，亮度在范围内随机跳变
		  //4=MEDIUM PLUSE  中速脉冲 以T_MP为周期，Lbright-Ldeep为下限，亮度在范围内随机跳变
		  //5=FAST PLUSE  快速脉冲   以T_FP为周期，Lbright-Ldeep为下限，亮度在范围内随机跳变
*/
#define T_SP 300	  //慢速脉冲模式，亮度变化周期，单位ms
#define T_MP 150	  //中速脉冲模式，亮度变化周期，单位ms
#define T_FP 75		  //快速脉冲模式，亮度变化周期，单位ms


/*typedef enum {
    TL_STATIC = 1,
    TL_SPARK = 2,
    TL_nSPARK = 3,
    TL_ELECTRICL = 4
}TriggerLMode;*/
/*
##TriggerLMode，触发模式以及可设参项

		  //1=STATIC  静止 保持亮度，色彩不变化  
		  //2=SPARK  闪烁，将当前BANK翻转为FBANK一次，时长为T_Spark
		  //3=nSPARK  n闪，将当前BANK翻转为FBANK nSparkCount次，单次时长为T_nSpark,间隔时间为T_nSparkGap
		  //4=ELECTRICL 持续闪烁   BANK与FBANK交替变化，单次保持时长为T_Electricl，亮度变化范围为Lbright至Lbright-Ldeep，
*/
#define TriggerLMode 1

#define T_Spark 200
#define nSparkCount 2
#define T_nSpark 150
#define T_nSparkGap 150
#define T_Electricl 100

#define PUT_MESSAGE_WAV_TIMEOUT 20
#define PUT_MESSAGE_LED_TIMEOUT 20

#define RESTART_VOLTAG	(1.62)
#define LOWPOWER_VOLTAG (1.53)
#define CC_VOLTAG       (2.1)
#define SYS_VOLTAG      (3.0)
#endif
