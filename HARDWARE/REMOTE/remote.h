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

// 红外遥控器按键值定义
#define KEY_POWER 	0x45
#define KEY_MODE  	0x46
#define KEY_MUTE  	0x47
#define KEY_PLAY  	0x44
#define KEY_PREV  	0x40
#define KEY_NEXT  	0x43
#define KEY_EQ    	0x07
#define KEY_VOL_SUB 0x15
#define KEY_VOL_ADD 0x09
#define KEY_0     	0x16
#define KEY_RPT   	0x19
#define KEY_USD   	0x0D
#define KEY_1     	0x0C
#define KEY_2     	0x18
#define KEY_3     	0x5E
#define KEY_4     	0x08
#define KEY_5     	0x1C
#define KEY_6     	0x5A
#define KEY_7     	0x42
#define KEY_8     	0x52
#define KEY_9     	0x4A

#endif
