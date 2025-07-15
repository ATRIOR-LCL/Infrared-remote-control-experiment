//////////////////////////////////////////////////////////////////////////////////	 
// 红外遥控LED调光系统 - 红外遥控头文件
// 功能说明：定义红外遥控相关的宏定义、函数声明和接口
// 兼容性设计：支持多种按键命名方式，便于代码移植和复用
// 按键映射：基于实际测试确定的NEC协议按键值
// 开发板：ALIENTEK STM32F4 NANO
// 技术支持：www.openedv.com
// 修改日期：2025-07-14
// 版本：V2.0（增加详细注释和兼容性说明）
//////////////////////////////////////////////////////////////////////////////////

#define RDATA   PBin(0)		// 红外数据输入引脚（PB0，连接红外接收头输出）

// 红外遥控器识别码(ID)
// 说明：每款遥控器都有唯一的识别码，用于区分不同品牌的遥控器
// 当前使用的遥控器识别码为0（ALIENTEK标准遥控器）
#define REMOTE_ID 0      		   

extern u8 RmtCnt;	        // 按键按下的次数计数器（外部变量声明）

// ==================== 函数声明 ====================
void Remote_Init(void);     // 红外接收模块初始化函数
u8 Remote_Scan(void);       // 红外按键扫描函数

// ==================== 标准按键值定义 ====================
// 以下按键值通过实际测试NEC协议解码得出，对应ALIENTEK标准遥控器
// 数字功能键组（主要控制功能）
#define KEY_POWER        162    // 电源键 - 系统模式切换/页面切换
#define KEY_NUM0         66     // 数字键0 - 控制LED0开关
#define KEY_NUM1         104    // 数字键1 - 控制LED1开关  
#define KEY_NUM2         152    // 数字键2 - 控制LED2开关
#define KEY_NUM3         176    // 数字键3 - 控制LED3开关
#define KEY_NUM4         48     // 数字键4 - 控制LED4开关
#define KEY_NUM5         24     // 数字键5 - 控制LED5开关
#define KEY_NUM6         122    // 数字键6 - 控制LED6开关
#define KEY_NUM7         16     // 数字键7 - 控制LED7开关
#define KEY_NUM8         56     // 数字键8 - 信息显示功能
#define KEY_NUM9         90     // 数字键9 - 控制所有LED切换

// 方向控制键组（亮度和功能控制）
#define KEY_UP           98     // 上键 - 增加LED亮度（支持长按连续调节）
#define KEY_DOWN         168    // 下键 - 降低LED亮度（支持长按连续调节）
#define KEY_LEFT         34     // 左键 - 扩展功能C（预留）
#define KEY_RIGHT        194    // 右键 - 扩展功能E（预留）
#define KEY_PLAY         2      // 播放键 - 扩展功能D（预留）

// 音量和特殊功能键组
#define KEY_VOL_UP       144    // 音量+键 - 扩展功能F（预留）
#define KEY_VOL_DOWN     224    // 音量-键 - 扩展功能G（预留）
#define KEY_DELETE       82     // 删除键 - 关闭所有LED
#define KEY_ALIENTEK     226    // ALIENTEK键 - 扩展功能H（预留）

// ==================== 兼容性宏定义区 ====================
// 说明：为了支持不同版本的实验代码和提高代码可移植性
//      定义了多套按键名称，它们指向相同的按键值

// IR_KEY_系列：兼容实验21等使用IR_KEY_前缀的代码
#define IR_KEY_POWER     KEY_POWER     // 162 - 电源键
#define IR_KEY_0         KEY_NUM0      // 66  - 数字键0
#define IR_KEY_1         KEY_NUM1      // 104 - 数字键1
#define IR_KEY_2         KEY_NUM2      // 152 - 数字键2
#define IR_KEY_3         KEY_NUM3      // 176 - 数字键3
#define IR_KEY_4         KEY_NUM4      // 48  - 数字键4
#define IR_KEY_5         KEY_NUM5      // 24  - 数字键5
#define IR_KEY_6         KEY_NUM6      // 122 - 数字键6
#define IR_KEY_7         KEY_NUM7      // 16  - 数字键7
#define IR_KEY_8         KEY_NUM8      // 56  - 数字键8
#define IR_KEY_9         KEY_NUM9      // 90  - 数字键9
#define IR_KEY_UP        KEY_UP        // 98  - 上键
#define IR_KEY_DOWN      KEY_DOWN      // 168 - 下键
#define IR_KEY_LEFT      KEY_LEFT      // 34  - 左键
#define IR_KEY_RIGHT     KEY_RIGHT     // 194 - 右键
#define IR_KEY_OK        KEY_PLAY      // 2   - 确认键（播放键）
#define IR_KEY_VOL_UP    KEY_VOL_UP    // 144 - 音量+键
#define IR_KEY_VOL_DN    KEY_VOL_DOWN  // 224 - 音量-键

// KEY_系列：兼容使用简化KEY_前缀的代码（省略NUM）
#define KEY_MODE         KEY_POWER     // 162 - 模式键（电源键别名）
#define KEY_MUTE         KEY_DELETE    // 82  - 静音键（删除键别名）
#define KEY_PREV         KEY_LEFT      // 34  - 上一首（左键别名）
#define KEY_NEXT         KEY_RIGHT     // 194 - 下一首（右键别名）
#define KEY_EQ           KEY_ALIENTEK  // 226 - 均衡器（ALIENTEK键别名）
#define KEY_VOL_SUB      KEY_VOL_DOWN  // 224 - 音量减（音量-键别名）
#define KEY_VOL_ADD      KEY_VOL_UP    // 144 - 音量加（音量+键别名）

// 简化数字键名称：KEY_0 等价于 KEY_NUM0
#define KEY_0            KEY_NUM0      // 66  - 数字键0
#define KEY_1            KEY_NUM1      // 104 - 数字键1
#define KEY_2            KEY_NUM2      // 152 - 数字键2
#define KEY_3            KEY_NUM3      // 176 - 数字键3
#define KEY_4            KEY_NUM4      // 48  - 数字键4
#define KEY_5            KEY_NUM5      // 24  - 数字键5
#define KEY_6            KEY_NUM6      // 122 - 数字键6
#define KEY_7            KEY_NUM7      // 16  - 数字键7
#define KEY_8            KEY_NUM8      // 56  - 数字键8
#define KEY_9            KEY_NUM9      // 90  - 数字键9

// 其他兼容性定义
#define KEY_RPT          KEY_PLAY      // 2   - 重复键（播放键别名）
#define KEY_USD          KEY_PLAY      // 2   - 自定义键（播放键别名）

// ==================== 使用示例和说明 ====================
/*
兼容性宏定义的使用示例：

方式1：使用标准名称（推荐）
if(key == KEY_NUM0) { LED_Toggle(0); }

方式2：使用IR_KEY_前缀（兼容实验21）
if(key == IR_KEY_0) { LED_Toggle(0); }

方式3：使用简化名称（兼容旧代码）
if(key == KEY_0) { LED_Toggle(0); }

三种写法完全等价，编译器会将它们都替换为：
if(key == 66) { LED_Toggle(0); }

这样设计的好处：
1. 代码移植性强：不同实验的代码可以直接移植使用
2. 可读性好：按键名称比数字更容易理解
3. 维护性强：只需要修改宏定义，不需要改动所有使用的地方
4. 向后兼容：旧代码无需修改即可使用新的头文件
*/

#endif
