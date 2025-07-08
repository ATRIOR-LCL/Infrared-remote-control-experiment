#ifndef __REMOTE_H
#define __REMOTE_H
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK NANO STM32F4开发板
//红外遥控接收 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2019/3/27
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2019-2029
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	

#define RDATA   PBin(0)		//红外数据输入脚

//红外遥控识别码(ID),每款遥控器的这个值基本都不一样,但也有一样的.
//我们选用的遥控器识别码为0
#define REMOTE_ID 0      		   

extern u8 RmtCnt;	        //按键按下的次数

void Remote_Init(void);     //红外传感器接收头引脚初始化
u8 Remote_Scan(void);

// 红外遥控器按键值定义（从实验21精确提取的按键值）
#define KEY_POWER        162    // 电源键 - 系统模式切换
#define KEY_NUM0         66     // 数字键0 - 控制LED0
#define KEY_NUM1         104    // 数字键1 - 控制LED1
#define KEY_NUM2         152    // 数字键2 - 控制LED2
#define KEY_NUM3         176    // 数字键3 - 控制LED3
#define KEY_NUM4         48     // 数字键4 - 控制LED4
#define KEY_NUM5         24     // 数字键5 - 控制LED5
#define KEY_NUM6         122    // 数字键6 - 控制LED6
#define KEY_NUM7         16     // 数字键7 - 控制LED7
#define KEY_NUM8         56     // 数字键8 - 显示8
#define KEY_NUM9         90     // 数字键9 - 控制所有LED

#define KEY_UP           98     // 上键 - 增加LED亮度
#define KEY_DOWN         168    // 下键 - 降低LED亮度
#define KEY_LEFT         34     // 左键 - 显示C
#define KEY_RIGHT        194    // 右键 - 显示E
#define KEY_PLAY         2      // 播放键 - 显示D
#define KEY_VOL_UP       144    // 音量+ - 显示F
#define KEY_VOL_DOWN     224    // 音量- - 显示小数点
#define KEY_DELETE       82     // 删除键 - 关闭所有LED
#define KEY_ALIENTEK     226    // ALIENTEK键 - 显示B

// IR_KEY_ 兼容性宏定义（为了与main.c兼容）
#define IR_KEY_POWER     KEY_POWER     // 162
#define IR_KEY_0         KEY_NUM0      // 66  
#define IR_KEY_1         KEY_NUM1      // 104
#define IR_KEY_2         KEY_NUM2      // 152
#define IR_KEY_3         KEY_NUM3      // 176
#define IR_KEY_4         KEY_NUM4      // 48
#define IR_KEY_5         KEY_NUM5      // 24
#define IR_KEY_6         KEY_NUM6      // 122
#define IR_KEY_7         KEY_NUM7      // 16
#define IR_KEY_8         KEY_NUM8      // 56
#define IR_KEY_9         KEY_NUM9      // 90
#define IR_KEY_UP        KEY_UP        // 98
#define IR_KEY_DOWN      KEY_DOWN      // 168
#define IR_KEY_LEFT      KEY_LEFT      // 34
#define IR_KEY_RIGHT     KEY_RIGHT     // 194
#define IR_KEY_OK        KEY_PLAY      // 2
#define IR_KEY_VOL_UP    KEY_VOL_UP    // 144
#define IR_KEY_VOL_DN    KEY_VOL_DOWN  // 224

// 兼容性定义（保持原有名称）
#define KEY_MODE         KEY_POWER
#define KEY_MUTE         KEY_DELETE
#define KEY_PREV         KEY_LEFT
#define KEY_NEXT         KEY_RIGHT
#define KEY_EQ           KEY_ALIENTEK
#define KEY_VOL_SUB      KEY_VOL_DOWN
#define KEY_VOL_ADD      KEY_VOL_UP
#define KEY_0            KEY_NUM0
#define KEY_1            KEY_NUM1
#define KEY_2            KEY_NUM2
#define KEY_3            KEY_NUM3
#define KEY_4            KEY_NUM4
#define KEY_5            KEY_NUM5
#define KEY_6            KEY_NUM6
#define KEY_7            KEY_NUM7
#define KEY_8            KEY_NUM8
#define KEY_9            KEY_NUM9
#define KEY_RPT          KEY_PLAY
#define KEY_USD          KEY_PLAY

#endif
