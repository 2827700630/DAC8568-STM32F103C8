/*
 * TI DAC7568/DAC8168/DAC8568 核心特性 (数据手册第1/5-7页)
 *
 * 【共同特性】
 * - 通道数量: 8通道独立电压输出
 * - 内部基准: 2.5V (±0.004%初始精度, 2ppm/°C温漂典型值)
 * - 毛刺能量: 0.1nV-s (代码切换瞬态)
 * - 输出缓冲: 轨到轨输出，驱动能力 20mA sink/source
 * - 电源范围: 2.7V至5.5V单电源供电
 * - 接口速度: 最高50MHz SPI时钟
 * - 低功耗:
 *   > 工作模式: 1.25mA @5V (含基准)
 *   > 断电模式: 0.18μA @5V (典型)
 * - 温度范围: -40°C至+125°C
 * - 复位特性: 上电复位至零电平/中间电平(型号后缀A/B/C/D区分)
 *
 * 【关键性能参数】
 * - DAC建立时间:
 *   > 5μs (典型值, 空载, 1/4至3/4量程, ±0.024%误差)
 *   > 10μs (最大值, 接1MΩ负载)
 *
 * 【型号差异】
 * >> DAC7568 (12位):
 *    - 分辨率: 12位 (0x000-0xFFF)
 *    - INL: ±1 LSB (最大值)
 *    - 封装: TSSOP-14/16
 *
 * >> DAC8168 (14位):
 *    - 分辨率: 14位 (0x0000-0x3FFF)
 *    - INL: ±4 LSB (最大值)
 *    - 封装: TSSOP-14/16
 *
 * >> DAC8568 (16位):
 *    - 分辨率: 16位 (0x0000-0xFFFF)
 *    - INL: ±12 LSB (最大值)
 *    - 封装: TSSOP-16
 *
 * 【编程注意】
 * 1. 数据对齐差异：
 *    DAC7568使用12位数据(D11-D0)，需左移4位填充到SPI帧
 *    DAC8168使用14位数据(D13-D0)，需左移2位
 *    DAC8568使用全部16位数据(D15-D0)
 *
 * 2. 建立时间影响：
 *    输出稳定需等待至少10μs，高速连续写入时需添加延时：
 *    HAL_Delay(1); // 保守延时1ms (实际最低10μs)
 */
/*
 * SPI 外设配置说明 (针对 DAC8568)
 * ----------------------------------------------------------------
 * 参数要求（参考数据手册第7-8页时序图和第6页电气特性）：
 * 1. 模式: 全双工主模式 (DAC8568 为从设备)
 * 2. 数据大小: 8位 (需手动组合为24/32位帧)
 * 3. 时钟极性 (CPOL): 低电平空闲 (CPOL=0)
 * 4. 时钟相位 (CPHA): 数据在第一个边沿采样 (CPHA=0)
 * 5. 传输顺序: MSB 高位在前 (强制要求)
 * 6. 波特率: ≤50MHz (建议根据主频选择分频器)
 * 7. 硬件NSS: 禁用 (使用独立GPIO控制SYNC引脚)
 *
 * CubeMX 具体设置步骤：
 * 1. Connectivity → SPIx → Mode = "Full-Duplex Master"
 * 2. Parameter Settings →
 *    - Data Size = 8 Bits
 *    - First Bit = MSB First
 *    - Clock Polarity = High  注意这个
 *    - Clock Phase = 1 Edge
 *    - Baud Rate Prescaler = 分频值 (如 2/4/8，确保 SCLK ≤50MHz)
 * 3. GPIO Settings →
 *    - 配置 SYNC 引脚为 GPIO_Output (初始化高电平)
 *    - MOSI/SCK 自动分配，模式为 Alternate Function Push-Pull
 */
/*
 * DAC8568 硬件连接说明：
 * ----------------------------------------------------------------
 * DAC8568引脚 | STM32引脚       | 备注
 * -----------|----------------|-----------------------------------
 * DIN        | SPIx_MOSI      | 主设备输出，直连无需上拉
 * SCLK       | SPIx_SCK       | 保持短走线，减少干扰
 * SYNC       | 任意GPIO (如PA4)| 需配置为输出模式，传输前拉低
 * VDD        | 3.3V           | 电源需并联100nF去耦电容
 * GND        | GND            | 共地
 */

#ifndef DAC8568_H
#define DAC8568_H

#include "main.h"

// DAC8568数据帧结构 (31-0位)
// 31-28位: 前缀位 (PREFIX BITS) - 始终为0
// 27-24位: 命令位 (CONTROL BITS)
// 23-20位: 地址位 (ADDRESS BITS)
// 19-4位:  数据位 (DATA BITS)
// 3-0位:   特征位 (FEATURE BITS)

// DAC8568命令定义 (27-24位控制位) (参考数据手册第35-38页表4)
#define CMD_WRITE_INPUT_REG 0b0000        // 写入输入寄存器
#define CMD_UPDATE_DAC_REG 0b0001         // 更新DAC寄存器
#define CMD_WRITE_INPUT_UPDATE_ALL 0b0010 // 写入所有输入寄存器并更新所有DAC寄存器
#define CMD_WRITE_INPUT_UPDATE_ONE 0b0011 // 写入单个输入寄存器并更新相应的DAC寄存器
#define CMD_POWER_DOWN 0b0100             // 电源模式控制
#define CMD_CLEAR_CODE_REG 0b0101         // 清除代码寄存器
#define CMD_LDAC_REG 0b0110               // LDAC寄存器控制
#define CMD_SOFTWARE_RESET 0b0111         // 软件复位
#define CMD_INTERNAL_REF 0b1000           // 内部参考控制

// DAC通道选择 (23-20位地址位) (参考数据手册第36页)
#define CHANNEL_A 0b0000 // 通道A
#define CHANNEL_B 0b0001 // 通道B
#define CHANNEL_C 0b0010 // 通道C
#define CHANNEL_D 0b0011 // 通道D
#define CHANNEL_E 0b0100 // 通道E
#define CHANNEL_F 0b0101 // 通道F
#define CHANNEL_G 0b0110 // 通道G
#define CHANNEL_H 0b0111 // 通道H
#define BROADCAST 0b1111 // 广播模式(所有通道)

// 电源模式定义 (特征位F1-F0) (参考数据手册第47页表13)
#define POWER_UP 0b00        // 正常工作模式
#define POWER_DOWN_1K 0b01   // 关断模式，1K电阻接地
#define POWER_DOWN_100K 0b10 // 关断模式，100K电阻接地
#define POWER_DOWN_HIZ 0b11  // 关断模式，高阻态

// 内部参考控制 (特征位) (参考数据手册第44-45页)
#define REF_DISABLE 0b0000              // 禁用内部参考
#define REF_ENABLE 0b0001               // 启用内部参考
#define REF_FLEX_MODE_ENABLE 0b0010     // 启用灵活模式
#define REF_FLEX_MODE_ALWAYS_ON 0b0011  // 灵活模式常开
#define REF_FLEX_MODE_ALWAYS_OFF 0b0100 // 灵活模式常关

// 清除代码控制 (特征位F1-F0) (参考数据手册第39页表5)
#define CLEAR_CODE_ZERO_SCALE 0b00   // 清除为零刻度
#define CLEAR_CODE_MID_SCALE 0b01    // 清除为中间刻度
#define CLEAR_CODE_FULL_SCALE 0b10   // 清除为全刻度
#define CLEAR_CODE_NO_OPERATION 0b11 // 无操作

// 函数声明
void DAC8568_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *sync_port, uint16_t sync_pin);
void DAC8568_Write(uint8_t channel, uint16_t data);
void DAC8568_Update(uint8_t channel);
void DAC8568_WriteAndUpdate(uint8_t channel, uint16_t data);
void DAC8568_WriteAllChannels(uint16_t *data);
void DAC8568_UpdateAllChannels(void);
void DAC8568_SetPowerMode(uint8_t channel, uint8_t mode);
void DAC8568_EnableInternalRef(uint8_t mode);
void DAC8568_SetClearCode(uint8_t mode);
void DAC8568_SoftwareReset(void);
void DAC8568_SendRawCommand(uint8_t cmd_bits, uint8_t addr_bits, uint16_t data_bits, uint8_t feature_bits);
void DAC8568_SendRawData(uint8_t raw_data[4]);

#endif /* DAC8568_H */