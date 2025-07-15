#include "remote.h"
#include "delay.h"
//////////////////////////////////////////////////////////////////////////////////	 
// 红外遥控LED调光系统 - 红外遥控驱动模块
// 功能说明：基于NEC协议的红外遥控信号接收和解码
// 硬件接口：PB0引脚连接红外接收头（TIM3_CH3输入捕获）
// 解码原理：通过定时器输入捕获测量红外信号的时间间隔
//          根据NEC协议的时间标准判断数据位"0"和"1"
// 开发板：ALIENTEK STM32F4 NANO
// 技术支持：www.openedv.com
// 开发团队：ALIENTEK团队
// 修改日期：2025-07-14
// 版本：V2.0（增加详细中文注释）
//////////////////////////////////////////////////////////////////////////////////

TIM_HandleTypeDef TIM3_Handler;      // 定时器3句柄（用于输入捕获）

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

// ==================== 红外遥控解码状态变量 ====================
// 红外遥控接收状态寄存器（8位状态标志）
// 位7 [7]：收到引导码标志（1=已收到9ms+4.5ms引导码，0=未收到）
// 位6 [6]：完整按键数据接收完成标志（1=完成，0=未完成）
// 位5 [5]：保留位，暂未使用
// 位4 [4]：上升沿捕获标记（1=已捕获上升沿，0=未捕获）
// 位3-0 [3:0]：溢出计时器（计数定时器更新中断次数，用于超时检测）
u8 	RmtSta=0;	  	  

// NEC协议解码相关变量
u16 Dval;		    // 下降沿时定时器的计数值（用于测量高电平持续时间）
u32 RmtRec=0;	    // 红外接收到的完整32位数据（地址码+地址反码+命令码+命令反码）
u8  RmtCnt=0;	    // 按键按下的次数计数器（用于连击检测） 

// ==================== 定时器中断服务函数 ====================
// 定时器3中断服务程序（入口函数）
// 功能：调用HAL库的通用中断处理函数，分发到具体的回调函数
void TIM3_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&TIM3_Handler);  // 调用HAL库定时器通用中断处理函数
} 

// ==================== NEC协议解码核心：定时器更新中断回调 ====================
// 定时器更新中断回调函数（超时检测和状态管理）
// 功能：每10ms触发一次，用于检测红外信号接收超时和状态清理
// 原理：NEC协议要求在特定时间内完成数据接收，超时则认为传输失败
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance==TIM3)  // 确认是定时器3触发的中断
	{
 		if(RmtSta&0x80)  // 检查是否已接收到引导码（位7=1）
		{	
			RmtSta&=~0X10;  // 清除上升沿捕获标记（位4=0）
			
			// 检查是否已完成一次完整的按键数据接收
			if((RmtSta&0X0F)==0X00)  // 计时器为0，说明接收完成
				RmtSta|=1<<6;  // 设置按键信息接收完成标志（位6=1）
			
			// 溢出计时器管理（防止长时间无信号导致状态锁定）
			if((RmtSta&0X0F)<14)  // 计时器未达到最大值
				RmtSta++;  // 计时器加1
			else  // 计时器达到最大值，超时处理
			{
				RmtSta&=~(1<<7);  // 清除引导码标志（位7=0）
				RmtSta&=0XF0;     // 清空溢出计时器（位3-0=0）
			}						 	   	
		}	
	}
}

// ==================== NEC协议解码核心：输入捕获中断回调 ====================
// 定时器输入捕获中断回调函数（NEC协议解码的核心逻辑）
// 功能：捕获红外信号的上升沿和下降沿，通过测量时间间隔解码NEC协议
// 原理：NEC协议通过不同的高电平持续时间来表示数据位"0"和"1"
//      数据位"0": 高电平560μs  数据位"1": 高电平1680μs
//      引导码: 高电平9ms + 低电平4.5ms
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance==TIM3)  // 确认是定时器3的输入捕获中断
	{
		if(RDATA)  // 检测到上升沿（红外信号从低电平变为高电平）
		{
			// ========== 上升沿处理：准备测量高电平持续时间 ==========
			TIM_RESET_CAPTUREPOLARITY(&TIM3_Handler,TIM_CHANNEL_3);   // 清除原有极性设置
			TIM_SET_CAPTUREPOLARITY(&TIM3_Handler,TIM_CHANNEL_3,TIM_ICPOLARITY_FALLING); // 设置为下降沿捕获
			__HAL_TIM_SET_COUNTER(&TIM3_Handler,0);  	// 重置定时器计数值为0，开始计时
		  	RmtSta|=0X10;  // 设置上升沿已捕获标志（位4=1）
		}
		else  // 检测到下降沿（红外信号从高电平变为低电平）
		{
			// ========== 下降沿处理：读取高电平持续时间并解码 ==========
			Dval=HAL_TIM_ReadCapturedValue(&TIM3_Handler,TIM_CHANNEL_3); // 读取高电平持续时间
			TIM_RESET_CAPTUREPOLARITY(&TIM3_Handler,TIM_CHANNEL_3);   	 // 清除原有极性设置
			TIM_SET_CAPTUREPOLARITY(&TIM3_Handler,TIM_CHANNEL_3,TIM_ICPOLARITY_RISING); // 设置为上升沿捕获
			
			if(RmtSta&0X10)  // 确认之前已捕获过上升沿（完成一次高电平测量）
			{
 				if(RmtSta&0X80)  // 已接收到引导码，开始数据位解码
				{
					// ========== 数据位解码部分 ==========
					if(Dval>300&&Dval<800)  // 高电平560μs±240μs → 数据位"0"
					{
						RmtRec<<=1;  // 接收数据左移1位，为新数据位腾出空间
						RmtRec|=0;   // 在最低位添加"0"
					}
					else if(Dval>1400&&Dval<1800)  // 高电平1680μs±200μs → 数据位"1"
					{
						RmtRec<<=1;  // 接收数据左移1位
						RmtRec|=1;   // 在最低位添加"1"
					}
					else if(Dval>2200&&Dval<2600)  // 高电平2500μs±200μs → 重复码
					{
						RmtCnt++;        // 按键重复次数加1
						RmtSta&=0XF0;    // 清空溢出计时器，重新开始计时
					}
 				}
				else if(Dval>4200&&Dval<4700)  // 高电平4500μs±250μs → 引导码
				{
					// ========== 引导码检测 ==========
					RmtSta|=1<<7;  // 设置引导码接收标志（位7=1）
					RmtCnt=0;      // 清除按键重复计数器，开始新的按键接收
				}						 
			}
			RmtSta&=~(1<<4);  // 清除上升沿捕获标志（位4=0），准备下次捕获
		}
	}
}

// ==================== NEC协议数据解析和按键扫描函数 ====================
// 红外遥控按键扫描函数
// 功能：解析接收到的32位NEC协议数据，提取并验证按键值
// 返回值：0=无按键按下，其他值=按下的按键编码
// 调用：在主循环中周期性调用，检测是否有新的按键按下
u8 Remote_Scan(void)
{        
	u8 sta=0;       // 最终返回的按键值
	u8 t1,t2;       // 临时变量，用于数据解析和校验
	
	if(RmtSta&(1<<6))  // 检查是否接收到完整的按键数据（位6=1）
	{ 
		// ========== NEC协议32位数据解析 ==========
		// NEC协议数据格式：[地址码8位][地址反码8位][命令码8位][命令反码8位]
		// 高位先传输，所以数据在RmtRec中的排列为：
		// 位31-24: 地址码    位23-16: 地址反码
		// 位15-8:  命令码    位7-0:   命令反码
		
	    t1=RmtRec>>24;			    // 提取地址码（位31-24）
	    t2=(RmtRec>>16)&0xff;	    // 提取地址反码（位23-16）
	    
	    // ========== 地址码验证 ==========
 	    if((t1==(u8)~t2)&&t1==REMOTE_ID)  // 验证：地址码 = ~地址反码 且 地址码 = 预设ID
	    { 
	        // 地址码验证通过，开始解析命令码
	        t1=RmtRec>>8;     // 提取命令码（位15-8）
	        t2=RmtRec; 	      // 提取命令反码（位7-0）
	        
	        // ========== 命令码验证 ==========
	        if(t1==(u8)~t2)   // 验证：命令码 = ~命令反码
	            sta=t1;       // 命令码验证通过，返回按键值
		}   
		
		// ========== 数据清理和状态重置 ==========
		if((sta==0)||((RmtSta&0X80)==0))  // 按键数据错误 或 遥控器已松开
		{
		 	RmtSta&=~(1<<6);  // 清除"接收到有效按键"标志（位6=0）
			RmtCnt=0;		  // 清除按键重复次数计数器
		}
	}  
    return sta;  // 返回按键值（0=无按键，其他=具体按键编码）
}
