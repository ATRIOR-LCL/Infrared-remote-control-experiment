#include "pwm.h"
#include "led.h"

//////////////////////////////////////////////////////////////////////////////////	 
// 红外遥控LED调光系统 - PWM驱动模块
// 功能说明：实现硬件PWM和软件PWM功能，用于LED亮度控制
// 硬件PWM：使用TIM1定时器产生PWM信号（可选功能）
// 软件PWM：使用TIM2定时器中断实现8路LED亮度调节
// 开发板：ALIENTEK STM32F4 NANO
// 版本：V1.0
// 日期：2025年7月
//////////////////////////////////////////////////////////////////////////////////

TIM_HandleTypeDef TIM2_Handler;         // 定时器2句柄（软件PWM用）

// 软件PWM相关变量
static u8 software_pwm_counter = 0;     // 软件PWM计数器（0-9循环计数）
static u8 led_brightness_duty = 5;      // LED亮度占空比（0-10级，5为中等亮度）

// TIM2 PWM初始化函数
// 功能：配置软件PWM定时器
// 说明：本项目主要使用软件PWM实现LED亮度调节
void TIM2_PWM_Init(u16 arr,u16 psc)
{
    // ============== 软件PWM定时器初始化 ==============
    // 使用TIM2定时器产生100Hz的中断，用于软件PWM控制
    TIM2_Handler.Instance = TIM2;                    // 选择定时器2
    TIM2_Handler.Init.Prescaler = 9599;              // 预分频：96MHz/(9599+1) = 10kHz
    TIM2_Handler.Init.CounterMode = TIM_COUNTERMODE_UP; // 向上计数模式
    TIM2_Handler.Init.Period = 99;                   // 重装载值：10kHz/100 = 100Hz中断频率
    TIM2_Handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1; // 不分频
    HAL_TIM_Base_Init(&TIM2_Handler);                // 初始化定时器基本功能
    
    // 启动定时器2中断，每10ms产生一次中断用于软件PWM
    HAL_TIM_Base_Start_IT(&TIM2_Handler);
}

//定时器基础功能底层驱动初始化函数
//功能：使能定时器时钟，配置NVIC中断优先级
//说明：此函数由HAL_TIM_Base_Init()自动调用，专门处理TIM2的底层配置
//htim：定时器句柄指针
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM2)              // 判断是否为定时器2
    {
        __HAL_RCC_TIM2_CLK_ENABLE();        // 使能定时器2时钟
        HAL_NVIC_SetPriority(TIM2_IRQn, 2, 0); // 设置TIM2中断优先级为2
        HAL_NVIC_EnableIRQ(TIM2_IRQn);      // 使能定时器2中断
    }
}

// LED亮度设置函数（0-10级亮度调节）
// 功能：设置软件PWM的占空比，实现LED亮度调节
// 参数：brightness_level - 亮度等级（0最暗，10最亮）
// 原理：通过改变PWM占空比来控制LED的平均功率，从而调节亮度
//      0级 = 占空比0%（完全熄灭），10级 = 占空比100%（最亮）
void LED_Brightness_Set(u8 brightness_level)
{
    if(brightness_level > 10) brightness_level = 10;  // 限制最大值为10
    led_brightness_duty = brightness_level;            // 保存亮度等级
}

// 软件PWM LED控制核心函数
// 功能：在定时器中断中调用，实现软件PWM波形生成
// 原理：每次中断counter递增，通过比较counter与duty值来控制LED开关
//      实现PWM波形：counter < duty时LED亮，counter >= duty时LED灭
// 调用：由TIM2中断每10ms调用一次，形成100Hz的PWM频率
void Software_PWM_LED_Control(void)
{
    software_pwm_counter++;                     // PWM计数器递增
    if(software_pwm_counter >= 10)              // 计数到10时归零（0-9循环）
        software_pwm_counter = 0;
    
    // 根据PWM占空比控制LED亮度（仅对开启的LED有效）
    // 从main.c引用LED状态数组，判断哪些LED是开启状态
    extern u8 led_status_array[8];             // 外部引用LED状态数组
    
    // 遍历8个LED，对每个开启的LED应用PWM控制
    for(u8 i = 0; i < 8; i++)
    {
        if(led_status_array[i] == 0)            // LED状态为0表示开启
        {
            // PWM波形生成：counter < duty时输出低电平（LED亮）
            if(software_pwm_counter < led_brightness_duty)
            {
                // 输出低电平，LED点亮（共阳极接法，低电平有效）
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
                // 输出高电平，LED熄灭
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

// 定时器2中断服务函数（软件PWM的核心）
// 功能：每10ms执行一次，产生软件PWM波形控制LED亮度
// 原理：在中断中调用Software_PWM_LED_Control()函数
//      通过定时器中断的精确定时，实现稳定的PWM频率
// 频率：100Hz（每10ms一次中断，PWM周期为100ms）
void TIM2_IRQHandler(void)
{
    // 检查定时器更新中断标志位
    if(__HAL_TIM_GET_FLAG(&TIM2_Handler, TIM_FLAG_UPDATE))
    {
        __HAL_TIM_CLEAR_FLAG(&TIM2_Handler, TIM_FLAG_UPDATE);  // 清除中断标志
        Software_PWM_LED_Control();                           // 执行软件PWM控制
    }
}
