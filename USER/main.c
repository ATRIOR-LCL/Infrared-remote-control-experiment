#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "spi.h"
#include "tftlcd.h"
#include "remote.h"
#include "pwm.h"

/************************************************
 ALIENTEK NANO STM32F4 IR Remote Control LCD Display Test
 Technical Support: www.openedv.com
 Author: ALIENTEK Team
 Modified: 2025-07-08
 Hardware: NANO STM32F4 Development Board
 ************************************************/

// Global variables
u8 led_status = 1;           // LED status: 1-off, 0-on
u8 led_brightness = 5;       // LED brightness level: 0-10
u8 current_page = 0;         // Current display page: 0-main, 1-LED control, 2-brightness
u8 all_leds_on = 0;         // All LEDs status

// LED control variables (compatible with experiment 21)
u8 led_status_array[8] = {1,1,1,1,1,1,1,1}; // LED status array, 1=off, 0=on
u8 all_led_status = 1; // All LEDs status, 1=off, 0=on
u8 led_brightness_level = 5; // LED brightness level, 0-10

// Key debounce variables (prevent repeated triggering)
u8 last_key = 0;         // Last key value
u8 key_repeat_count = 0; // Key repeat count
u8 key_debounce_timer = 0; // Key debounce timer

// Function declarations
void Display_Main_Page(void);
void Display_LED_Control_Page(void);
void Display_Brightness_Page(void);
void Update_LED_Display(void);
void Process_Remote_Key(u8 key);
void LED_All_Control(u8 state);
void Show_Key_Info_New(u8 key);

// Experiment 21 compatible function declarations
void LED_Toggle(u8 led_num);
void LED_All_Toggle(void);
void LED_All_Set(u8 status);
void LED_Brightness_Up(void);
void LED_Brightness_Down(void);
void System_Mode_Switch(void);

int main(void)
{ 
    u8 key=0;   // Key value

    HAL_Init();                     // Initialize HAL library    
    Stm32_Clock_Init(96,4,2,4);     // Set clock to 96MHz
    delay_init(96);                 // Initialize delay function
    uart_init(115200);              // Initialize UART 115200
    LED_Init();                     // Initialize LED 	
    LCD_Init();                     // Initialize LCD
    Remote_Init();                  // Initialize IR remote  
    TIM1_PWM_Init(1000-1,96-1);     // Initialize PWM for LED brightness
    
    // Display main page
    Display_Main_Page();
	
	while(1)
	{
		key = Remote_Scan();        // 扫描红外按键
		
		// 防抖计时器递减
		if(key_debounce_timer > 0) key_debounce_timer--;
		
		if(key != 0)
		{
			// 按键防抖：只有在防抖时间过后，或者是新按键才处理
			if(key != last_key)
			{
				// 新按键，重置防抖和重复计数
				last_key = key;
				key_repeat_count = 0;
				key_debounce_timer = 10; // 100ms防抖时间（10*10ms）
				
				// 显示按键信息和处理按键
				Show_Key_Info_New(key);
				printf("Key Value: 0x%02X (%d)\r\n", key, key);
				Process_Remote_Key(key);
			}
			else if(key_debounce_timer == 0)
			{
				// 同一按键，检查重复计数
				key_repeat_count++;
				
				// 对于UP/DOWN键，实现延时重复触发（用于亮度连续调节）
				if((key == 98 || key == 168) && key_repeat_count >= 25) // 250ms后开始重复
				{
					key_repeat_count = 20; // 重置为较小值，加快重复速度
					Show_Key_Info_New(key);
					printf("Key Value: 0x%02X (%d) [Repeat]\r\n", key, key);
					Process_Remote_Key(key);
				}
				else if(key_repeat_count > 1 && key != 98 && key != 168)
				{
					// 其他按键不允许快速重复，忽略
					key = 0;
				}
			}
		}
		else
		{
			// 无按键时，清除重复计数
			if(last_key != 0)
			{
				last_key = 0;
				key_repeat_count = 0;
			}
		}
		
		delay_ms(10);
	}
}

// ��ʾ��ҳ
void Display_Main_Page(void)
{
	LCD_Clear(BLACK);
	Display_ALIENTEK_LOGO(0, 0);
	
	POINT_COLOR = WHITE;
	BACK_COLOR = BLACK;
	LCD_ShowString(10, 80, 240, 16, 16, "IR Remote Control");
	LCD_ShowString(10, 100, 240, 16, 16, "LED & LCD System");
	
	POINT_COLOR = YELLOW;
	LCD_ShowString(10, 130, 240, 12, 12, "Key Functions:");
	LCD_ShowString(10, 145, 240, 12, 12, "0-7: LED Control");
	LCD_ShowString(10, 158, 240, 12, 12, "9: All LEDs Toggle");
	LCD_ShowString(10, 171, 240, 12, 12, "UP/DOWN: Brightness");
	LCD_ShowString(10, 184, 240, 12, 12, "POWER: Switch Page");
	LCD_ShowString(10, 197, 240, 12, 12, "DELETE: All LEDs OFF");
	
	current_page = 0;
	Update_LED_Display();
}

// ��ʾLED����ҳ
void Display_LED_Control_Page(void)
{
	LCD_Clear(BLUE);
	POINT_COLOR = WHITE;
	BACK_COLOR = BLUE;
	
	LCD_ShowString(10, 10, 240, 16, 16, "LED Control Page");
	LCD_ShowString(10, 40, 240, 12, 12, "Current LED Status:");
	
	current_page = 1;
	Update_LED_Display();
}

// ��ʾ������ҳ
void Display_Brightness_Page(void)
{
	LCD_Clear(GREEN);
	POINT_COLOR = WHITE;
	BACK_COLOR = GREEN;
	
	LCD_ShowString(10, 10, 240, 16, 16, "Brightness Control");
	LCD_ShowString(10, 40, 240, 12, 12, "Current Level:");
	
	current_page = 2;
	Update_LED_Display();
}

// ����LED��ʾ
void Update_LED_Display(void)
{
	char str[50];
	
	if(current_page == 0) // ��ҳ
	{
		POINT_COLOR = GREEN;
		BACK_COLOR = BLACK;
		sprintf(str, "LED: %s  Brightness: %d/10", 
				led_status ? "OFF" : "ON", led_brightness);
		LCD_ShowString(10, 215, 240, 12, 12, str);
	}
	else if(current_page == 1) // LED����ҳ
	{
		POINT_COLOR = YELLOW;
		BACK_COLOR = BLUE;
		sprintf(str, "LED0-7: %s", all_leds_on ? "ALL ON" : "SINGLE");
		LCD_ShowString(10, 60, 240, 12, 12, str);
		
		sprintf(str, "Current: %s", led_status ? "OFF" : "ON");
		LCD_ShowString(10, 75, 240, 12, 12, str);
	}
	else if(current_page == 2) // ������ҳ
	{
		POINT_COLOR = BLACK;
		BACK_COLOR = GREEN;
		sprintf(str, "%d / 10", led_brightness);
		LCD_ShowString(150, 40, 240, 16, 16, str);
		
		// ��ʾ������������
		for(int i = 0; i < 10; i++)
		{
			if(i < led_brightness)
				LCD_Fill(10 + i*20, 70, 25 + i*20, 85, WHITE);
			else
				LCD_Fill(10 + i*20, 70, 25 + i*20, 85, BLACK);
		}
	}
}

// ����ң������
void Process_Remote_Key(u8 key)
{
    switch(key)
    {
        case 66:  // KEY_NUM0 - 控制LED0
            LED_Toggle(0); 
            Show_Key_Info_New(key);
            break;
            
        case 104: // KEY_NUM1 - 控制LED1  
            LED_Toggle(1);
            Show_Key_Info_New(key);
            break;
            
        case 152: // KEY_NUM2 - 控制LED2
            LED_Toggle(2);
            Show_Key_Info_New(key);
            break;
            
        case 176: // KEY_NUM3 - 控制LED3
            LED_Toggle(3);
            Show_Key_Info_New(key);
            break;
            
        case 48:  // KEY_NUM4 - 控制LED4
            LED_Toggle(4);
            Show_Key_Info_New(key);
            break;
            
        case 24:  // KEY_NUM5 - 控制LED5
            LED_Toggle(5);
            Show_Key_Info_New(key);
            break;
            
        case 122: // KEY_NUM6 - 控制LED6
            LED_Toggle(6);
            Show_Key_Info_New(key);
            break;
            
        case 16:  // KEY_NUM7 - 控制LED7
            LED_Toggle(7);
            Show_Key_Info_New(key);
            break;
            
        case 56:  // KEY_NUM8 - 显示信息
            Show_Key_Info_New(key);
            break;
            
        case 90:  // KEY_NUM9 - 控制所有LED
            LED_All_Toggle();
            Show_Key_Info_New(key);
            break;
            
        case 82:  // KEY_DELETE - 关闭所有LED
            LED_All_Set(1); // 1表示关闭
            Show_Key_Info_New(key);
            break;
            
        case 162: // KEY_POWER - 切换系统模式
            System_Mode_Switch();
            Show_Key_Info_New(key);
            break;
            
        case 98:  // KEY_UP - 增加LED亮度
            LED_Brightness_Up();
            Show_Key_Info_New(key);
            break;
            
        case 168: // KEY_DOWN - 降低LED亮度
            LED_Brightness_Down();
            Show_Key_Info_New(key);
            break;
            
        case 34:  // KEY_LEFT - 功能C
            Show_Key_Info_New(key);
            break;
            
        case 2:   // KEY_PLAY - 功能D
            Show_Key_Info_New(key);
            break;
            
        case 194: // KEY_RIGHT - 功能E
            Show_Key_Info_New(key);
            break;
            
        case 144: // KEY_VOL_UP - 功能F
            Show_Key_Info_New(key);
            break;
            
        case 224: // KEY_VOL_DOWN - 小数点显�?
            Show_Key_Info_New(key);
            break;
            
        case 226: // KEY_ALIENTEK - 功能B
            Show_Key_Info_New(key);
            break;
            
        default:
            break;
    }
}

// ����ȫ��LED
void LED_All_Control(u8 state)
{
	LED0 = state;
	LED1 = state;
	LED2 = state;
	LED3 = state;
	LED4 = state;
	LED5 = state;
	LED6 = state;
	LED7 = state;
}

// LED控制函数（与实验21兼容）
void LED_Toggle(u8 led_num)
{
    if(led_num < 8)
    {
        led_status_array[led_num] = !led_status_array[led_num];
        
        // 直接控制LED状态，亮度由软件PWM自动处理
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
        
        // 软件PWM会自动处理亮度，无需手动调用LED_Brightness_Set
    }
}

void LED_All_Toggle(void)
{
    all_led_status = !all_led_status;
    LED_All_Set(all_led_status);
}

void LED_All_Set(u8 status)
{
    all_led_status = status;
    for(u8 i = 0; i < 8; i++)
    {
        led_status_array[i] = status;
    }
    
    LED0 = status;
    LED1 = status;
    LED2 = status;
    LED3 = status;
    LED4 = status;
    LED5 = status;
    LED6 = status;
    LED7 = status;
    
    // 软件PWM会自动处理亮度控制
}

void LED_Brightness_Up(void)
{
    if(led_brightness_level < 10)
    {
        led_brightness_level++;
        led_brightness = led_brightness_level; // 同步旧变量
        
        // 更新软件PWM亮度设置
        LED_Brightness_Set(led_brightness_level);
        Update_LED_Display();
    }
}

void LED_Brightness_Down(void)
{
    if(led_brightness_level > 0)
    {
        led_brightness_level--;
        led_brightness = led_brightness_level; // 同步旧变量
        
        // 更新软件PWM亮度设置
        LED_Brightness_Set(led_brightness_level);
        Update_LED_Display();
    }
}

void System_Mode_Switch(void)
{
    // 切换页面显示模式（与原有逻辑兼容�?
    current_page = (current_page + 1) % 3;
    if(current_page == 0)
        Display_Main_Page();
    else if(current_page == 1)
        Display_LED_Control_Page();
    else
        Display_Brightness_Page();
}

// ��ʾ����Ϣ
// 显示按键信息（不使用strcat避免警告�?
void Show_Key_Info_New(u8 key)
{
	char str[50];
	
	POINT_COLOR = RED;
	BACK_COLOR = WHITE;
	
	// 清除位置，准备显示按键信�?
	LCD_Fill(10, 225, 230, 240, WHITE);
	
	// 使用sprintf直接生成完整字符串，避免strcat警告
	switch(key)
	{
		case 162: sprintf(str, "Key:0x%02X POWER", key); break;
		case 66:  sprintf(str, "Key:0x%02X NUM0", key); break;
		case 104: sprintf(str, "Key:0x%02X NUM1", key); break;
		case 152: sprintf(str, "Key:0x%02X NUM2", key); break;
		case 176: sprintf(str, "Key:0x%02X NUM3", key); break;
		case 48:  sprintf(str, "Key:0x%02X NUM4", key); break;
		case 24:  sprintf(str, "Key:0x%02X NUM5", key); break;
		case 122: sprintf(str, "Key:0x%02X NUM6", key); break;
		case 16:  sprintf(str, "Key:0x%02X NUM7", key); break;
		case 56:  sprintf(str, "Key:0x%02X NUM8", key); break;
		case 90:  sprintf(str, "Key:0x%02X NUM9", key); break;
		case 98:  sprintf(str, "Key:0x%02X UP", key); break;
		case 168: sprintf(str, "Key:0x%02X DOWN", key); break;
		case 144: sprintf(str, "Key:0x%02X VOL+", key); break;
		case 224: sprintf(str, "Key:0x%02X VOL-", key); break;
		case 82:  sprintf(str, "Key:0x%02X DELETE", key); break;
		case 34:  sprintf(str, "Key:0x%02X LEFT", key); break;
		case 194: sprintf(str, "Key:0x%02X RIGHT", key); break;
		case 2:   sprintf(str, "Key:0x%02X PLAY", key); break;
		case 226: sprintf(str, "Key:0x%02X ALIENTEK", key); break;
		default:  sprintf(str, "Key:0x%02X Unknown", key); break;
	}
	
	LCD_ShowString(10, 227, 220, 12, 12, str);
}

