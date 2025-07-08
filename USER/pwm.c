#include "pwm.h"
#include "led.h"

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

TIM_HandleTypeDef TIM1_Handler;         //定时器句柄
TIM_HandleTypeDef TIM2_Handler;         //定时器2句柄用于软件PWM

// 软件PWM相关变量
static u8 software_pwm_counter = 0;     // 软件PWM计数器
static u8 led_brightness_duty = 5;      // LED亮度占空比(0-10)

//TIM1 PWM部分初始化 
//arr：自动重装值
//psc：时钟预分频数
void TIM1_PWM_Init(u16 arr,u16 psc)
{  
    // 初始化硬件PWM（可选，如果需要的话）
    TIM_OC_InitTypeDef TIM1_CH1Handler;	    //定时器1通道1句柄
	
    TIM1_Handler.Instance=TIM1;                      //定时器1
    TIM1_Handler.Init.Prescaler=psc;                 //定时器分频
    TIM1_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;//向上计数模式
    TIM1_Handler.Init.Period=arr;                    //自动重装载值
    TIM1_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_PWM_Init(&TIM1_Handler);                 //初始化PWM
    
    TIM1_CH1Handler.OCMode=TIM_OCMODE_PWM1;          //模式选择PWM1
    TIM1_CH1Handler.Pulse=arr/2;                     //设置比较值,此值用来确定占空比，默认比较值为自动重装载值的一半,即占空比为50%
    TIM1_CH1Handler.OCPolarity=TIM_OCPOLARITY_HIGH;  //输出比较极性为高 
    HAL_TIM_PWM_ConfigChannel(&TIM1_Handler,&TIM1_CH1Handler,TIM_CHANNEL_1);//配置TIM1通道1
    HAL_TIM_PWM_Start(&TIM1_Handler,TIM_CHANNEL_1);  //开启PWM通道1
    
    // 初始化软件PWM定时器TIM2
    TIM2_Handler.Instance = TIM2;
    TIM2_Handler.Init.Prescaler = 9599;              // 分频至10kHz (96MHz/9600 = 10kHz)
    TIM2_Handler.Init.CounterMode = TIM_COUNTERMODE_UP;
    TIM2_Handler.Init.Period = 99;                   // 100次计数 = 100Hz PWM频率
    TIM2_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&TIM2_Handler);
    
    // 使能定时器2中断
    HAL_TIM_Base_Start_IT(&TIM2_Handler);
}

//定时器底层驱动，开启时钟，设置中断优先级
//此函数会被HAL_TIM_PWM_Init()调用
//htim:定时器句柄
void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_Initure;
    __HAL_RCC_TIM1_CLK_ENABLE();			//使能定时器1
    __HAL_RCC_GPIOA_CLK_ENABLE();			//开启GPIOA时钟
    
    GPIO_Initure.Pin=GPIO_PIN_8;           	//PA8
    GPIO_Initure.Mode=GPIO_MODE_AF_PP;  	//复用推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //高速
    GPIO_Initure.Alternate= GPIO_AF1_TIM1;	//PA8复用为TIM1通道1
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);
}

//定时器底层驱动，开启时钟，设置中断优先级
//此函数会被HAL_TIM_Base_Init()调用
//htim:定时器句柄
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM2)
    {
        __HAL_RCC_TIM2_CLK_ENABLE();        //使能定时器2时钟
        HAL_NVIC_SetPriority(TIM2_IRQn, 2, 0); //设置中断优先级
        HAL_NVIC_EnableIRQ(TIM2_IRQn);      //使能定时器2中断
    }
}

//设置TIM1通道1的占空比
//duty：占空比设置
void LED_PWM_Set_Duty(u8 led_num, u16 duty)
{
    __HAL_TIM_SET_COMPARE(&TIM1_Handler,TIM_CHANNEL_1,duty);
}

// LED亮度设置（0-10级）- 使用软件PWM控制LED亮度
void LED_Brightness_Set(u8 brightness_level)
{
    if(brightness_level > 10) brightness_level = 10;
    led_brightness_duty = brightness_level;
}

// 软件PWM LED控制函数 - 在定时器中断中调用
void Software_PWM_LED_Control(void)
{
    software_pwm_counter++;
    if(software_pwm_counter >= 10) 
        software_pwm_counter = 0;
    
    // 根据占空比控制LED状态（仅对开启的LED有效）
    // 这个函数需要知道哪些LED当前是开启状态的
    extern u8 led_status_array[8];  // 引用main.c中的LED状态数组
    
    for(u8 i = 0; i < 8; i++)
    {
        if(led_status_array[i] == 0)  // 如果LED开启
        {
            if(software_pwm_counter < led_brightness_duty)
            {
                // LED应该点亮
                switch(i)
                {
                    case 0: LED0 = 0; break;  // 0表示点亮
                    case 1: LED1 = 0; break;
                    case 2: LED2 = 0; break;
                    case 3: LED3 = 0; break;
                    case 4: LED4 = 0; break;
                    case 5: LED5 = 0; break;
                    case 6: LED6 = 0; break;
                    case 7: LED7 = 0; break;
                }
            }
            else
            {
                // LED应该熄灭
                switch(i)
                {
                    case 0: LED0 = 1; break;  // 1表示熄灭
                    case 1: LED1 = 1; break;
                    case 2: LED2 = 1; break;
                    case 3: LED3 = 1; break;
                    case 4: LED4 = 1; break;
                    case 5: LED5 = 1; break;
                    case 6: LED6 = 1; break;
                    case 7: LED7 = 1; break;
                }
            }
        }
    }
}

// 定时器2中断服务函数
void TIM2_IRQHandler(void)
{
    if(__HAL_TIM_GET_FLAG(&TIM2_Handler, TIM_FLAG_UPDATE))
    {
        __HAL_TIM_CLEAR_FLAG(&TIM2_Handler, TIM_FLAG_UPDATE);
        Software_PWM_LED_Control();  // 执行软件PWM控制
    }
}
