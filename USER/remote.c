#include "remote.h"
#include "delay.h"
//////////////////////////////////////////////////////////////////////////////////	 
// This program is for learning purposes only.
// ALIENTEK NANO STM32F4 Development Board
// Infrared Remote Control Driver Code	   
// ALIENTEK Team
// Forum: www.openedv.com
// Modified: 2019/3/27
// Version: V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2019-2029
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////  	

TIM_HandleTypeDef TIM3_Handler;      //定时器3句柄

//红外遥控初始化
//设置IO以及TIM3_CH3输入捕获
void Remote_Init(void)
{  
    TIM_IC_InitTypeDef TIM3_CH3Config;  
    
    TIM3_Handler.Instance=TIM3;                          //通用定时器3
    TIM3_Handler.Init.Prescaler=(96-1);                	 //预分频器,1M的计数频率,1us计1.
    TIM3_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;    //向上计数器
    TIM3_Handler.Init.Period=10000;                      //自动装载值
    TIM3_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;//时钟分频因子
    HAL_TIM_IC_Init(&TIM3_Handler);
    
    //初始化TIM3输入捕获参数
    TIM3_CH3Config.ICPolarity=TIM_ICPOLARITY_RISING;    //上升沿捕获
    TIM3_CH3Config.ICSelection=TIM_ICSELECTION_DIRECTTI;//映射到TI3上
    TIM3_CH3Config.ICPrescaler=TIM_ICPSC_DIV1;          //配置输入分频,不分频
    TIM3_CH3Config.ICFilter=0x03;                       //IC4F=0003 8个定时器时钟周期滤波
    HAL_TIM_IC_ConfigChannel(&TIM3_Handler,&TIM3_CH3Config,TIM_CHANNEL_3);//配置TIM3通道3
    HAL_TIM_IC_Start_IT(&TIM3_Handler,TIM_CHANNEL_3);   //开始捕获TIM3的通道3
    __HAL_TIM_ENABLE_IT(&TIM3_Handler,TIM_IT_UPDATE);   //使能更新中断
}

//定时器3底层驱动，时钟使能，引脚配置
//此函数会被HAL_TIM_IC_Init()调用
//htim:定时器3句柄
void HAL_TIM_IC_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_TIM3_CLK_ENABLE();            //使能TIM3时钟
    __HAL_RCC_GPIOB_CLK_ENABLE();			//开启GPIOB时钟
	
    GPIO_Initure.Pin=GPIO_PIN_0;            //PB0
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;  	 //复用推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //高速
	GPIO_Initure.Alternate=GPIO_AF2_TIM3;   //PB0复用为TIM3通道3
    HAL_GPIO_Init(GPIOB,&GPIO_Initure);

    HAL_NVIC_SetPriority(TIM3_IRQn,1,3); 	//设置中断优先级，抢占优先级1，子优先级3
    HAL_NVIC_EnableIRQ(TIM3_IRQn);       	//开启ITM3中断
}

//遥控器接收状态
//[7]:收到了引导码标志
//[6]:得到了一个按键的所有信息
//[5]:保留	
//[4]:标记上升沿是否已经被捕获								   
//[3:0]:溢出计时器
u8 	RmtSta=0;	  	  
u16 Dval;		//下降沿时计数器的值
u32 RmtRec=0;	//红外接收到的数据	   		    
u8  RmtCnt=0;	//按键按下的次数	 

//定时器3中断服务程序
void TIM3_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TIM3_Handler);//定时器共用处理函数
} 

//定时器更新中断回调函数
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance==TIM3)
	{
 		if(RmtSta&0x80)//上次有数据被接收到了
		{	
			RmtSta&=~0X10;						//取消上升沿已经被捕获标记
			if((RmtSta&0X0F)==0X00)RmtSta|=1<<6;//标记已经完成一次按键的键值信息采集
			if((RmtSta&0X0F)<14)RmtSta++;
			else
			{
				RmtSta&=~(1<<7);//清空引导标识
				RmtSta&=0XF0;	//清空计数器	
			}						 	   	
		}	
	}
}

//定时器输入捕获中断回调函数
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)//捕获中断发生时执行
{
	if(htim->Instance==TIM3)
	{
		if(RDATA)//上升沿捕获
		{
			TIM_RESET_CAPTUREPOLARITY(&TIM3_Handler,TIM_CHANNEL_3);   //一定要先清除原来的设置！！
			TIM_SET_CAPTUREPOLARITY(&TIM3_Handler,TIM_CHANNEL_3,TIM_ICPOLARITY_FALLING);//CC1P=1 设置为下降沿捕获
			__HAL_TIM_SET_COUNTER(&TIM3_Handler,0);  	//清空定时器值   	  
		  	RmtSta|=0X10;							//标记上升沿已经被捕获
		}else //下降沿捕获
		{
			Dval=HAL_TIM_ReadCapturedValue(&TIM3_Handler,TIM_CHANNEL_3);//读取CCR2也可以清CC2IF标志位
			TIM_RESET_CAPTUREPOLARITY(&TIM3_Handler,TIM_CHANNEL_3);   	//一定要先清除原来的设置！！
			TIM_SET_CAPTUREPOLARITY(&TIM3_Handler,TIM_CHANNEL_3,TIM_ICPOLARITY_RISING);//配置TIM3通道3上升沿捕获
			if(RmtSta&0X10)							//完成一次高电平捕获 
			{
 				if(RmtSta&0X80)//接收到了引导码
				{
					
					if(Dval>300&&Dval<800)			//560为标准值,560us
					{
						RmtRec<<=1;					//左移一位.
						RmtRec|=0;					//接收到0	   
					}else if(Dval>1400&&Dval<1800)	//1680为标准值,1680us
					{
						RmtRec<<=1;					//左移一位.
						RmtRec|=1;					//接收到1
					}else if(Dval>2200&&Dval<2600)	//得到按键键值增加的信息 2500为标准值2.5ms
					{
						RmtCnt++; 					//按键次数增加1次
						RmtSta&=0XF0;				//清空计时器		
					}
 				}else if(Dval>4200&&Dval<4700)		//4500为标准值4.5ms
				{
					RmtSta|=1<<7;					//标记成功接收到了引导码
					RmtCnt=0;						//清除按键次数计数器
				}						 
			}
			RmtSta&=~(1<<4);
		}
	}
}

//处理红外键盘
//返回值:
//	 0,没有任何按键按下
//其他,按下的按键键值.
u8 Remote_Scan(void)
{        
	u8 sta=0;       
	u8 t1,t2;  
	if(RmtSta&(1<<6))//得到一个按键的所有信息了
	{ 
	    t1=RmtRec>>24;			//得到地址码
	    t2=(RmtRec>>16)&0xff;	//得到地址反码 
 	    if((t1==(u8)~t2)&&t1==REMOTE_ID)//检验遥控识别码(ID)及地址 
	    { 
	        t1=RmtRec>>8;
	        t2=RmtRec; 	
	        if(t1==(u8)~t2)sta=t1;//键值正确	 
		}   
		if((sta==0)||((RmtSta&0X80)==0))//按键数据错误/遥控已经没有按下了
		{
		 	RmtSta&=~(1<<6);//清除接收到有效按键标识
			RmtCnt=0;		//清除按键次数计数器
		}
	}  
    return sta;
}
