#ifndef __PWM_H
#define __PWM_H
#include "sys.h"

//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK NANO STM32F4开发板
//PWM输出驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2019-2029
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////

void TIM1_PWM_Init(u16 arr,u16 psc);
void LED_PWM_Set_Duty(u8 led_num, u16 duty);
void LED_Brightness_Set(u8 brightness_level);

#endif
