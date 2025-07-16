#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "spi.h"
#include "tftlcd.h"
#include "remote.h"
#include "pwm.h"

/************************************************
 红外遥控LED调光系统 - 主程序文件
 功能说明：实现红外遥控器控制8路LED独立开关和亮度调节
 硬件平台：ALIENTEK STM32F4 NANO开发板
 显示设备：1.3寸TFTLCD彩色显示屏
 控制方式：红外遥控器18键功能控制
 主要功能：
 - 8路LED独立控制（数字键0-7）
 - LED亮度10级调节（UP/DOWN键）
 - 三页面LCD显示切换（POWER键）
 - 按键防抖和长按连续调节
 - 软件PWM实现LED亮度控制
 技术支持：www.openedv.com
 开发团队：ALIENTEK团队
 修改日期：2025-07-05
 ************************************************/

// ==================== 全局变量定义区 ====================
u8 led_status = 1;           // 主LED状态：1-关闭，0-开启（兼容旧版本）
u8 led_brightness = 5;       // LED亮度等级：0-10级（兼容旧版本）
u8 current_page = 0;         // 当前显示页面：0-主页面，1-LED控制页，2-亮度控制页

// LED控制变量组（兼容实验21的接口）
u8 led_status_array[8] = {1,1,1,1,1,1,1,1}; // LED状态数组：1=关闭，0=开启
u8 all_led_status = 1;       // 所有LED的统一状态：1=关闭，0=开启
u8 led_brightness_level = 5; // LED亮度等级：0-10级（0最暗，10最亮）

// 按键防抖变量组（防止按键重复触发导致的误操作）
u8 last_key = 0;         // 上一次按键值，用于检测按键变化
u8 key_repeat_count = 0; // 按键重复计数器，用于实现长按功能
u8 key_debounce_timer = 0; // 按键防抖定时器，消除按键抖动

// ==================== 函数声明区 ====================
// LCD显示相关函数
void Display_Main_Page(void);           // 显示主页面
void Display_LED_Control_Page(void);    // 显示LED控制页面
void Display_Brightness_Page(void);     // 显示亮度控制页面
void Update_LED_Display(void);          // 更新LED状态显示
void Show_Key_Info_New(u8 key);         // 显示按键信息

// 系统控制相关函数
void Process_Remote_Key(u8 key);        // 处理红外遥控按键

// 实验21兼容函数（保持接口兼容性）
void LED_Toggle(u8 led_num);            // 切换指定LED状态
void LED_All_Toggle(void);              // 切换所有LED状态
void LED_All_Set(u8 status);            // 设置所有LED状态
void LED_Brightness_Up(void);           // 增加LED亮度
void LED_Brightness_Down(void);         // 降低LED亮度
void System_Mode_Switch(void);          // 系统模式切换

// ==================== 主函数 ====================
int main(void)
{ 
    u8 key=0;   // 红外遥控按键值

    // ========== 系统初始化阶段 ==========
    HAL_Init();                     // 初始化HAL库（硬件抽象层）
    Stm32_Clock_Init(96,4,2,4);     // 配置系统时钟为96MHz
    delay_init(96);                 // 初始化延时函数（基于96MHz时钟）
    uart_init(115200);              // 初始化串口通信（波特率115200）
    LED_Init();                     // 初始化8路LED硬件接口
    LCD_Init();                     // 初始化1.3寸TFTLCD显示屏
    Remote_Init();                  // 初始化红外遥控接收模块
    TIM2_PWM_Init(1000-1,96-1);     // 初始化软件PWM定时器（用于LED亮度控制）
    
    // 显示系统启动主页面
    Display_Main_Page();
	
	// ========== 主循环：按键扫描与处理 ==========
	while(1)
	{
		key = Remote_Scan();        // 扫描红外遥控按键值
		
		// ========== 按键防抖定时器管理 ==========
		// 防抖定时器每个循环递减1，10ms后自动归零
		// 作用：确保按键信号稳定后才允许处理，避免机械抖动干扰
		if(key_debounce_timer > 0) key_debounce_timer--;
		
		if(key != 0)  // 检测到按键按下
		{
			// ========== 核心按键防抖算法 ==========
			/*
			 * 防抖算法工作原理：
			 * 1. 新按键检测：当前按键值与上次不同时，立即处理
			 * 2. 相同按键防抖：设置防抖时间窗口，避免抖动重复触发
			 * 3. 长按重复：防抖时间过后允许特定按键重复触发
			 * 4. 状态保持：记录按键状态，用于下次比较
			 */
			
			if(key != last_key)  // 条件1：检测到新按键（不同于上次按键）
			{
				// ========== 新按键立即处理分支 ==========
				/*
				 * 新按键处理逻辑：
				 * - 立即响应：新按键无需等待，提升用户体验
				 * - 状态重置：清除所有防抖和重复计数状态
				 * - 防抖启动：设置防抖时间窗口，防止后续抖动
				 */
				last_key = key;                  // 保存当前按键值，用于下次比较
				key_repeat_count = 0;            // 重置重复计数器，清除长按状态
				key_debounce_timer = 10;         // 设置100ms防抖时间（10个循环×10ms）
				
				// 执行按键功能（新按键立即响应）
				Show_Key_Info_New(key);          // 在LCD上显示按键信息
				printf("Key Value: 0x%02X (%d)\r\n", key, key);  // 串口输出调试信息
				Process_Remote_Key(key);         // 执行按键对应的功能
			}
			else if(key_debounce_timer == 0)  // 条件2：相同按键且防抖时间已过
			{
				// ========== 长按重复触发算法 ==========
				/*
				 * 长按重复机制说明：
				 * - 防抖完成：防抖时间归零，确认按键信号稳定
				 * - 重复计数：累计相同按键的持续时间
				 * - 选择性重复：只有特定按键（UP/DOWN）支持长按重复
				 * - 重复频率控制：通过计数器实现不同的重复速度
				 */
				key_repeat_count++;              // 重复计数递增，记录按键持续时间
				
				// UP/DOWN键支持长按连续调节亮度功能
				if((key == 98 || key == 168) && key_repeat_count >= 25) // 250ms后开始重复
				{
					/*
					 * 长按重复触发条件：
					 * - 按键类型：仅UP(98)和DOWN(168)键支持
					 * - 触发时间：持续按下250ms（25个循环×10ms）
					 * - 重复频率：每200ms重复一次（20个循环间隔）
					 */
					key_repeat_count = 20;       // 重置为较小值，实现200ms重复间隔
					Show_Key_Info_New(key);      // 显示重复按键信息
					printf("Key Value: 0x%02X (%d) [Repeat]\r\n", key, key);
					Process_Remote_Key(key);     // 执行重复功能（亮度连续调节）
				}
				else if(key_repeat_count > 1 && key != 98 && key != 168)
				{
					/*
					 * 其他按键防重复机制：
					 * - 目的：防止数字键等功能键意外重复触发
					 * - 方法：检测到重复后强制清零按键值
					 * - 范围：除UP/DOWN键外的所有按键
					 */
					key = 0;  // 强制清零，忽略后续重复触发
				}
			}
			// 注意：当key == last_key且key_debounce_timer > 0时
			// 程序不进入任何分支，直接忽略该次按键，实现防抖效果
		}
		else  // 没有按键按下（key == 0）
		{
			// ========== 按键释放状态清理 ==========
			/*
			 * 按键释放处理：
			 * - 状态重置：清除所有按键相关的状态变量
			 * - 准备下次：为下一次按键检测做准备
			 * - 防止误判：避免释放状态影响后续按键检测
			 */
			if(last_key != 0)  // 检查是否有按键状态需要清理
			{
				last_key = 0;           // 清除上次按键记录，表示无按键按下
				key_repeat_count = 0;   // 清除重复计数，准备接收新按键
				// 注意：防抖定时器会自然递减归零，无需手动清除
			}
		}
		
		delay_ms(10);  // 主循环延时10ms，控制扫描频率和时间基准
	}
}

// ==================== LCD页面显示函数组 ====================
/*
 * LCD显示系统设计说明：
 * 硬件：1.3寸TFTLCD彩色显示屏，分辨率240x240像素
 * 接口：SPI通信协议，高速数据传输
 * 颜色：16位RGB565格式，支持65536种颜色
 * 坐标：(0,0)为左上角，X轴向右，Y轴向下
 * 功能：三页面设计，支持文字、图形、进度条显示
 */

// 显示系统主页面
// 功能：显示ALIENTEK Logo、系统标题和按键功能说明
// 调用：系统启动时和页面切换时调用
// 设计：黑色背景+彩色文字，突出专业感和可读性
void Display_Main_Page(void)
{
	// ========== 页面背景和Logo显示 ==========
	/*
	 * LCD_Clear()函数原理：
	 * - 功能：将整个屏幕填充为指定颜色
	 * - 实现：通过SPI快速写入240*240个像素点
	 * - 颜色：BLACK = 0x0000（16位RGB565格式）
	 */
	LCD_Clear(BLACK);                    // 清屏为黑色背景，提供高对比度
	
	/*
	 * Display_ALIENTEK_LOGO()函数说明：
	 * - 功能：显示预定义的ALIENTEK公司Logo图形
	 * - 参数：(0, 0)表示显示位置为左上角
	 * - 实现：通过像素数组或字符方式绘制Logo
	 * - 作用：品牌标识和系统启动提示
	 */
	Display_ALIENTEK_LOGO(0, 0);         // 在屏幕左上角显示ALIENTEK Logo
	
	// ========== 系统标题显示区域 ==========
	/*
	 * LCD颜色设置机制：
	 * POINT_COLOR：文字前景色（字体颜色）
	 * BACK_COLOR：文字背景色（字符底色）
	 * 颜色格式：16位RGB565 (R:5位, G:6位, B:5位)
	 * 常用颜色：BLACK(0x0000), WHITE(0xFFFF), RED(0xF800), GREEN(0x07E0), BLUE(0x001F)
	 */
	POINT_COLOR = WHITE;                 // 设置字体颜色为白色，在黑背景上清晰可见
	BACK_COLOR = BLACK;                  // 设置字符背景为黑色，与屏幕背景一致
	
	/*
	 * LCD_ShowString()函数详解：
	 * 参数1：x坐标（像素）- 字符串起始X位置
	 * 参数2：y坐标（像素）- 字符串起始Y位置  
	 * 参数3：宽度（像素）- 显示区域最大宽度，用于换行控制
	 * 参数4：高度（像素）- 显示区域最大高度，用于垂直范围控制
	 * 参数5：字体大小 - 字符高度（12/16/24等），影响显示效果
	 * 参数6：字符串内容 - 要显示的文本内容
	 */
	LCD_ShowString(10, 80, 240, 16, 16, "IR Remote Control");   // 系统标题第1行
	LCD_ShowString(10, 100, 240, 16, 16, "LED & LCD System");   // 系统标题第2行
	
	// ========== 功能说明文字显示区域 ==========
	/*
	 * 颜色切换策略：
	 * - 白色标题：突出系统名称的重要性
	 * - 黄色说明：功能描述用不同颜色区分，提升可读性
	 * - 颜色对比：确保在黑色背景上有良好的视觉效果
	 */
	POINT_COLOR = YELLOW;                // 切换为黄色字体，用于功能说明文字
	
	/*
	 * 功能说明布局设计：
	 * Y坐标间距：13-15像素，确保文字不重叠且排列整齐
	 * 字体大小：12像素高度，适合详细说明文字
	 * 内容安排：按使用频率和重要性排列功能说明
	 */
	LCD_ShowString(10, 130, 240, 12, 12, "Key Functions:");      // 功能说明标题
	LCD_ShowString(10, 145, 240, 12, 12, "0-7: LED Control");    // 数字键0-7：LED控制
	LCD_ShowString(10, 158, 240, 12, 12, "9: All LEDs Toggle");  // 数字键9：所有LED切换
	LCD_ShowString(10, 171, 240, 12, 12, "UP/DOWN: Brightness"); // UP/DOWN键：亮度调节
	LCD_ShowString(10, 184, 240, 12, 12, "POWER: Switch Page");  // POWER键：页面切换
	LCD_ShowString(10, 197, 240, 12, 12, "DELETE: All LEDs OFF");// DELETE键：关闭所有LED
	
	// ========== 页面状态设置 ==========
	current_page = 0;                    // 设置当前页面标识为主页面(0)
	Update_LED_Display();                // 调用状态显示函数，在页面底部显示LED状态
}

// 显示LED控制页面
// 功能：专门用于显示LED控制状态的页面
// 背景：蓝色主题，突出LED控制功能
// 设计：简洁明了，突出LED状态信息的可视化
void Display_LED_Control_Page(void)
{
	// ========== LED控制页面背景设置 ==========
	/*
	 * 页面主题色彩设计原理：
	 * - 蓝色背景：BLUE = 0x001F，代表技术和控制的专业感
	 * - 白色文字：在蓝色背景上提供最佳对比度和可读性
	 * - 功能分区：通过颜色区分不同的功能模块
	 */
	LCD_Clear(BLUE);                     // 清屏为蓝色背景，营造控制面板氛围
	POINT_COLOR = WHITE;                 // 设置字体颜色为白色，确保高对比度
	BACK_COLOR = BLUE;                   // 设置字符背景为蓝色，与屏幕背景一致
	
	// ========== LED控制页面标题和状态显示 ==========
	/*
	 * 页面布局设计：
	 * - 顶部：页面标题，使用16像素大字体突出显示
	 * - 中部：LED状态信息区域，用于显示当前LED控制模式
	 * - 底部：由Update_LED_Display()函数动态更新具体状态
	 */
	LCD_ShowString(10, 10, 240, 16, 16, "LED Control Page");     // 页面标题，位置靠上
	LCD_ShowString(10, 40, 240, 12, 12, "Current LED Status:");  // LED状态提示文字
	
	current_page = 1;                    // 设置当前页面标识为LED控制页(1)
	Update_LED_Display();                // 调用状态更新函数，显示具体LED控制信息
}

// 显示亮度控制页面  
// 功能：专门用于显示LED亮度调节的页面
// 背景：绿色主题，突出亮度调节功能
// 特色：包含可视化亮度进度条，直观显示当前亮度等级
void Display_Brightness_Page(void)
{
	// ========== 亮度控制页面背景设置 ==========
	/*
	 * 绿色主题设计理念：
	 * - 绿色背景：GREEN = 0x07E0，象征能量和亮度调节
	 * - 黑色文字：在绿色背景上提供清晰的视觉对比
	 * - 色彩心理：绿色给用户带来舒适和自然的感受
	 */
	LCD_Clear(GREEN);                    // 清屏为绿色背景，突出亮度调节主题
	POINT_COLOR = WHITE;                 // 设置字体颜色为白色，保持清晰可读
	BACK_COLOR = GREEN;                  // 设置字符背景为绿色，保持界面一致性
	
	// ========== 亮度控制页面内容显示 ==========
	/*
	 * 亮度页面特殊设计：
	 * - 标题简洁：直接说明页面功能
	 * - 数值显示：Current Level标签为后续数值显示做准备
	 * - 进度条：由Update_LED_Display()绘制可视化亮度条
	 */
	LCD_ShowString(10, 10, 240, 16, 16, "Brightness Control");   // 页面标题
	LCD_ShowString(10, 40, 240, 12, 12, "Current Level:");       // 亮度等级提示文字
	
	current_page = 2;                    // 设置当前页面标识为亮度控制页(2)
	Update_LED_Display();                // 调用更新函数，显示亮度数值和进度条
}

// 更新LED状态显示函数
// 功能：根据当前页面显示相应的LED状态信息
// 调用：在LED状态改变或页面切换时调用
// 特点：多页面自适应显示，包含文字信息和图形化进度条
void Update_LED_Display(void)
{
	char str[50];  // 字符串缓冲区，用于sprintf格式化显示内容
	
	// ========== 主页面状态显示模式 ==========
	if(current_page == 0)  // 主页面显示模式
	{
		/*
		 * 主页面状态显示设计：
		 * - 位置：屏幕底部(Y=215)，不干扰主要内容
		 * - 颜色：绿色文字突出状态信息的重要性
		 * - 内容：LED总体状态 + 当前亮度等级
		 * - 格式：简洁明了，一行显示所有关键信息
		 */
		POINT_COLOR = GREEN;                     // 设置状态信息为绿色，表示系统正常
		BACK_COLOR = BLACK;                      // 背景保持黑色，与主页面一致
		
		/*
		 * sprintf格式化字符串原理：
		 * - %s：字符串占位符，根据条件显示"OFF"或"ON"
		 * - %d：整数占位符，显示亮度数值
		 * - 三元运算符：led_status ? "OFF" : "ON" 实现状态文字切换
		 * - 拼接结果：如"LED: ON  Brightness: 7/10"
		 */
		sprintf(str, "LED: %s  Brightness: %d/10", 
				led_status ? "OFF" : "ON", led_brightness);
		LCD_ShowString(10, 215, 240, 12, 12, str);  // 在屏幕底部显示状态信息
	}
	
	// ========== LED控制页面详细显示模式 ==========
	else if(current_page == 1)  // LED控制页面显示模式
	{
		/*
		 * LED控制页面显示特色：
		 * - 颜色：黄色突出控制状态，在蓝色背景上醒目
		 * - 内容：显示LED控制模式和当前状态
		 * - 层次：两行信息，第一行显示控制模式，第二行显示当前状态
		 */
		POINT_COLOR = YELLOW;                    // 黄色字体在蓝色背景上形成鲜明对比
		BACK_COLOR = BLUE;                       // 保持与页面背景一致的蓝色
		
		/*
		 * LED控制模式显示逻辑：
		 * - "ALL ON"：所有LED统一控制模式
		 * - "SINGLE"：单个LED独立控制模式  
		 * - all_leds_on变量：控制模式标志位
		 */
		sprintf(str, "LED0-7: %s", all_leds_on ? "ALL ON" : "SINGLE");
		LCD_ShowString(10, 60, 240, 12, 12, str);  // Y=60位置显示控制模式
		
		/*
		 * LED当前状态显示：
		 * - 显示所有LED的总体开关状态
		 * - 位置：紧接在控制模式下方
		 */
		sprintf(str, "Current: %s", led_status ? "OFF" : "ON");
		LCD_ShowString(10, 75, 240, 12, 12, str);  // Y=75位置显示当前状态
	}
	
	// ========== 亮度控制页面可视化显示模式 ==========
	else if(current_page == 2)  // 亮度控制页面显示模式
	{
		/*
		 * 亮度页面显示设计亮点：
		 * 1. 数值显示：直观的数字亮度等级
		 * 2. 进度条：10段式图形化亮度指示
		 * 3. 颜色对比：黑色文字在绿色背景上清晰可见
		 */
		POINT_COLOR = BLACK;                     // 黑色字体在绿色背景上清晰易读
		BACK_COLOR = GREEN;                      // 保持绿色背景的主题一致性
		
		/*
		 * 亮度数值显示：
		 * - 位置：X=150，与"Current Level:"标签对齐
		 * - 格式：当前亮度/最大亮度 (如"7 / 10")
		 * - 字体：16像素，比标签更大，突出重要性
		 */
		sprintf(str, "%d / 10", led_brightness);
		LCD_ShowString(150, 40, 240, 16, 16, str);  // 显示亮度数值
		
		// ========== 亮度进度条绘制算法 ==========
		/*
		 * 10段式亮度进度条设计：
		 * - 总长度：200像素 (10段 × 20像素/段)
		 * - 每段宽度：15像素 (20像素间距 - 5像素间隔)
		 * - 高度：15像素 (Y=70到Y=85)
		 * - 颜色：白色表示已点亮段，黑色表示未点亮段
		 * - 位置：水平居中，垂直在数值下方
		 */
		for(int i = 0; i < 10; i++)  // 循环绘制10个亮度段
		{
			/*
			 * LCD_Fill()函数详解：
			 * 参数1-2：起始坐标 (x1, y1)
			 * 参数3-4：结束坐标 (x2, y2) 
			 * 参数5：填充颜色
			 * 功能：在指定矩形区域填充指定颜色
			 * 
			 * 坐标计算：
			 * - X起始：10 + i*20 (每段间隔20像素)
			 * - X结束：25 + i*20 (每段宽度15像素)
			 * - Y固定：70-85 (高度15像素)
			 */
			if(i < led_brightness)  // 判断当前段是否应该点亮
			{
				// 当前亮度范围内的段显示为白色（点亮状态）
				LCD_Fill(10 + i*20, 70, 25 + i*20, 85, WHITE);
			}
			else
			{
				// 超出当前亮度的段显示为黑色（熄灭状态）
				LCD_Fill(10 + i*20, 70, 25 + i*20, 85, BLACK);
			}
		}
		/*
		 * 进度条视觉效果：
		 * 亮度0：全黑 ■■■■■■■■■■
		 * 亮度5：半亮 □□□□□■■■■■  
		 * 亮度10：全亮 □□□□□□□□□□
		 * 实时更新：每次亮度改变都会重新绘制
		 */
	}
}

// ==================== 红外遥控按键处理函数 ====================
// 红外遥控按键处理主函数
// 功能：根据接收到的红外按键值执行相应的控制功能
// 参数：key - 红外遥控器按键对应的数值编码
// 说明：每个按键对应不同的功能，实现LED控制、亮度调节、页面切换等
void Process_Remote_Key(u8 key)
{
    switch(key)
    {
        // ========== 数字键0-7：独立LED控制 ==========
        case 66:  // 数字键0 - 控制LED0
            LED_Toggle(0); 
            Show_Key_Info_New(key);
            break;
            
        case 104: // 数字键1 - 控制LED1  
            LED_Toggle(1);
            Show_Key_Info_New(key);
            break;
            
        case 152: // 数字键2 - 控制LED2
            LED_Toggle(2);
            Show_Key_Info_New(key);
            break;
            
        case 176: // 数字键3 - 控制LED3
            LED_Toggle(3);
            Show_Key_Info_New(key);
            break;
            
        case 48:  // 数字键4 - 控制LED4
            LED_Toggle(4);
            Show_Key_Info_New(key);
            break;
            
        case 24:  // 数字键5 - 控制LED5
            LED_Toggle(5);
            Show_Key_Info_New(key);
            break;
            
        case 122: // 数字键6 - 控制LED6
            LED_Toggle(6);
            Show_Key_Info_New(key);
            break;
            
        case 16:  // 数字键7 - 控制LED7
            LED_Toggle(7);
            Show_Key_Info_New(key);
            break;
            
        case 56:  // 数字键8 - 显示按键信息（功能预留）
            Show_Key_Info_New(key);
            break;
            
        // ========== 系统控制功能键 ==========
        case 90:  // 数字键9 - 控制所有LED同时切换
            LED_All_Toggle();
            Show_Key_Info_New(key);
            break;
            
        case 82:  // DELETE键 - 关闭所有LED
            LED_All_Set(1);               // 1表示关闭所有LED
            Show_Key_Info_New(key);
            break;
            
        case 162: // POWER键 - 切换显示页面模式
            System_Mode_Switch();
            Show_Key_Info_New(key);
            break;
            
        // ========== 亮度调节功能键 ==========
        case 98:  // UP键 - 增加LED亮度（支持长按连续调节）
            LED_Brightness_Up();
            Show_Key_Info_New(key);
            break;
            
        case 168: // DOWN键 - 降低LED亮度（支持长按连续调节）
            LED_Brightness_Down();
            Show_Key_Info_New(key);
            break;
            
        // ========== 其他功能键（预留扩展功能） ==========
        case 34:  // LEFT键 - 功能C（预留）
            Show_Key_Info_New(key);
            break;
            
        case 2:   // PLAY键 - 功能D（预留）
            Show_Key_Info_New(key);
            break;
            
        case 194: // RIGHT键 - 功能E（预留）
            Show_Key_Info_New(key);
            break;
            
        case 144: // VOL+键 - 功能F（预留）
            Show_Key_Info_New(key);
            break;
            
        case 224: // VOL-键 - 功能G（预留）
            Show_Key_Info_New(key);
            break;
            
        case 226: // ALIENTEK键 - 功能H（预留）
            Show_Key_Info_New(key);
            break;
            
        default:  // 未定义按键，不执行任何操作
            break;
    }
}

// ==================== 实验21兼容接口函数组 ====================

// 单个LED状态切换函数
// 功能：切换指定编号LED的开关状态（开→关 或 关→开）
// 参数：led_num - LED编号（0-7）
// 原理：修改状态数组，然后直接控制对应硬件IO口
//      软件PWM会自动根据状态数组判断是否需要亮度控制
void LED_Toggle(u8 led_num)
{
    if(led_num < 8)  // 检查LED编号有效性
    {
        led_status_array[led_num] = !led_status_array[led_num];  // 切换状态
        
        // 直接控制对应LED硬件，亮度由软件PWM自动处理
        switch(led_num)
        {
            case 0: LED0 = led_status_array[0]; break;
            case 1: LED1 = led_status_array[1]; break;
            case 2: LED2 = led_status_array[2]; break;
            case 3: LED3 = led_status_array[3]; break;
            case 4: LED4 = led_status_array[4]; break;
            case 5: LED5 = led_status_array[5]; break;
            case 6: LED6 = led_status_array[6]; break;
            case 7: LED7 = led_status_array[7]; break;
        }
        
        // 注意：软件PWM会自动处理亮度控制，无需手动调用LED_Brightness_Set
    }
}

// 所有LED状态切换函数
// 功能：同时切换所有LED的开关状态
// 原理：先切换全局状态标志，然后调用统一设置函数
void LED_All_Toggle(void)
{
    all_led_status = !all_led_status;    // 切换全局LED状态
    LED_All_Set(all_led_status);         // 应用到所有LED
}

// 所有LED状态设置函数
// 功能：将所有LED设置为指定状态
// 参数：status - 目标状态（1=关闭，0=开启）
// 原理：更新状态数组和全局状态，然后统一控制硬件
void LED_All_Set(u8 status)
{
    all_led_status = status;             // 更新全局状态标志
    
    // 更新所有LED的状态数组
    for(u8 i = 0; i < 8; i++)
    {
        led_status_array[i] = status;
    }
    
    // 直接控制所有LED硬件
    LED0 = status;
    LED1 = status;
    LED2 = status;
    LED3 = status;
    LED4 = status;
    LED5 = status;
    LED6 = status;
    LED7 = status;
    
    // 软件PWM会自动根据状态数组处理亮度控制
}

// LED亮度增加函数
// 功能：将LED亮度等级增加1级（支持长按连续调节）
// 范围：0-10级，到达最高亮度时停止增加
// 原理：修改亮度变量，调用软件PWM设置函数，更新显示
void LED_Brightness_Up(void)
{
    if(led_brightness_level < 10)           // 检查是否已达最大亮度
    {
        led_brightness_level++;             // 亮度等级加1
        led_brightness = led_brightness_level; // 同步旧版本变量（兼容性）
        
        // 更新软件PWM亮度设置
        LED_Brightness_Set(led_brightness_level);
        Update_LED_Display();               // 更新LCD显示
    }
}

// LED亮度降低函数
// 功能：将LED亮度等级降低1级（支持长按连续调节）
// 范围：0-10级，到达最低亮度时停止降低
// 原理：修改亮度变量，调用软件PWM设置函数，更新显示
void LED_Brightness_Down(void)
{
    if(led_brightness_level > 0)            // 检查是否已达最小亮度
    {
        led_brightness_level--;             // 亮度等级减1
        
        // 更新软件PWM亮度设置
        LED_Brightness_Set(led_brightness_level);
        Update_LED_Display();               // 更新LCD显示
    }
}

// 系统显示模式切换函数
// 功能：在三个显示页面间循环切换（主页面→LED控制页→亮度控制页→主页面...）
// 调用：POWER键按下时调用
// 原理：使用取模运算实现循环切换，根据页面编号调用对应显示函数
void System_Mode_Switch(void)
{
    // 页面编号循环切换（0→1→2→0...）
    current_page = (current_page + 1) % 3;
    
    // 根据新的页面编号显示对应页面
    if(current_page == 0)
        Display_Main_Page();                // 显示主页面
    else if(current_page == 1)
        Display_LED_Control_Page();         // 显示LED控制页面
    else
        Display_Brightness_Page();          // 显示亮度控制页面
}

// ==================== 按键信息显示函数 ====================
/*
 * 按键信息显示系统设计说明：
 * 功能：实时显示用户按下的红外遥控按键信息
 * 位置：屏幕底部固定区域，不干扰主要内容显示
 * 格式：十六进制按键码 + 英文按键名称
 * 更新：每次按键都会清除旧信息并显示新信息
 * 作用：用户反馈、系统调试、按键映射确认
 */

// 按键信息显示函数（避免使用strcat防止编译警告）
// 功能：在LCD屏幕底部显示当前按下的红外遥控按键信息
// 参数：key - 红外遥控按键的数值编码
// 显示：按键编码（十六进制）+ 按键名称（英文）
// 位置：屏幕底部白色背景区域，红色字体
void Show_Key_Info_New(u8 key)
{
	char str[50];  // 字符串缓冲区，存储格式化后的按键信息
	POINT_COLOR = RED;                       // 设置字体颜色为红色，突出按键信息
	BACK_COLOR = WHITE;                      // 设置背景颜色为白色，形成强烈对比
	LCD_Fill(10, 225, 230, 240, WHITE);     // 清除按键信息显示区域
	switch(key)
	{
		case 162: sprintf(str, "Key:0x%02X POWER", key); break;    // POWER键：页面切换功能
		case 66:  sprintf(str, "Key:0x%02X NUM0", key); break;     // 数字0：LED0控制
		case 104: sprintf(str, "Key:0x%02X NUM1", key); break;     // 数字1：LED1控制
		case 152: sprintf(str, "Key:0x%02X NUM2", key); break;     // 数字2：LED2控制
		case 176: sprintf(str, "Key:0x%02X NUM3", key); break;     // 数字3：LED3控制
		case 48:  sprintf(str, "Key:0x%02X NUM4", key); break;     // 数字4：LED4控制
		case 24:  sprintf(str, "Key:0x%02X NUM5", key); break;     // 数字5：LED5控制
		case 122: sprintf(str, "Key:0x%02X NUM6", key); break;     // 数字6：LED6控制
		case 16:  sprintf(str, "Key:0x%02X NUM7", key); break;     // 数字7：LED7控制
		case 56:  sprintf(str, "Key:0x%02X NUM8", key); break;     // 数字8：信息显示功能
		case 90:  sprintf(str, "Key:0x%02X NUM9", key); break;     // 数字9：所有LED切换
		case 98:  sprintf(str, "Key:0x%02X UP", key); break;       // UP键：亮度增加
		case 168: sprintf(str, "Key:0x%02X DOWN", key); break;     // DOWN键：亮度降低
		case 34:  sprintf(str, "Key:0x%02X LEFT", key); break;     // LEFT键：预留功能
		case 194: sprintf(str, "Key:0x%02X RIGHT", key); break;    // RIGHT键：预留功能
		case 2:   sprintf(str, "Key:0x%02X PLAY", key); break;     // PLAY键：预留功能
		case 144: sprintf(str, "Key:0x%02X VOL+", key); break;     // 音量+：预留功能
		case 224: sprintf(str, "Key:0x%02X VOL-", key); break;     // 音量-：预留功能
		case 82:  sprintf(str, "Key:0x%02X DELETE", key); break;   // DELETE键：关闭所有LED
		case 226: sprintf(str, "Key:0x%02X ALIENTEK", key); break; // ALIENTEK键：预留功能
		default:  sprintf(str, "Key:0x%02X Unknown", key); break; // 未定义的按键
	}
	LCD_ShowString(10, 227, 220, 12, 12, str);  // 在屏幕底部显示按键信息
}
