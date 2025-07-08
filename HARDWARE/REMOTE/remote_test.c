/*
 * 红外遥控器按键测试程序
 * 用于调试和验证红外遥控器功能
 */

#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "remote.h"

// 简单的红外测试函数
void IR_Remote_Test(void)
{
    u8 key = 0;
    
    printf("红外遥控器测试程序\r\n");
    printf("请按遥控器按键...\r\n");
    
    while(1)
    {
        key = Remote_Scan();
        if(key != 0)
        {
            printf("接收到按键: 0x%02X\r\n", key);
            
            switch(key)
            {
                case KEY_POWER:
                    printf("按键: POWER\r\n");
                    break;
                case KEY_MODE:
                    printf("按键: MODE\r\n");
                    break;
                case KEY_1:
                    printf("按键: 1\r\n");
                    break;
                case KEY_2:
                    printf("按键: 2\r\n");
                    break;
                case KEY_3:
                    printf("按键: 3\r\n");
                    break;
                case KEY_VOL_ADD:
                    printf("按键: VOL+\r\n");
                    break;
                case KEY_VOL_SUB:
                    printf("按键: VOL-\r\n");
                    break;
                default:
                    printf("按键: 未知 (0x%02X)\r\n", key);
                    break;
            }
            
            delay_ms(200); // 防止重复检测
        }
        
        delay_ms(10);
    }
}

/*
 * 使用方法：
 * 1. 在main函数中调用 IR_Remote_Test() 函数
 * 2. 通过串口监视器查看按键检测结果
 * 3. 确认红外遥控器工作正常后再集成到主程序
 */
