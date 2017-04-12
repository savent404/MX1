# MX_AudioPlayer
A audioplayer with Acceleration sensor and Play with fun
## Change History
### V1.4
修改GPIO口使得LED硬件顺序与配置文件中的顺序相符合
### V1.3
由于硬件变更导致的一系列的修改：
* 普通LED灯（非功率型）由4路增加到8路
* 音频的音量控制由IO口硬件控制功放输出功率的形式改为调整DAC输出信号的幅度

bug修复
* 在运行模式下超快速点击AUX键会误触发Colorswitch而不是TriggerD
* 在定义中Out音效和In音效的文件最大限制量错误地交换了，在本该由Out_Max控制的量变为In_Max控制
* Cube文件中对FreeRTOS的部分参数与程序中的定义不相符

其他修改
* 更新了HAL代码库到更新的版本
* 更改了功率级led的显示效果以满足用户需求
* 删去了保护代码


## How to use
You needs 

>[MDK Version 5](http://www2.keil.com/mdk5) 包括STM32F1的支持包

>[STM32CUBE MX](http://www.stm32cube.com/)  提供工程配置的管理

>[Visual Studio Code](https://code.visualstudio.com/) 進行代碼編輯

>[Source Light](https://www.baidu.com) 針對代碼結構的檢查

### OS support
>[FreeRTOS](http://www.freertos.org/)
### About Audio Format
>[PCM WAVE Format](http://ibillxia.github.io/blog/2013/07/20/details-of-wave-format-and-reading-wave-files-in-C-language/)

### Function check

**[OK]Main Button**

**[OK]Aux Button**

**[OK]Main&Aux**

**[OK]SD card SETTING**
