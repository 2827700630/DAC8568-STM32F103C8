#include "dac8568.h"

static SPI_HandleTypeDef *hspi_dac;
static GPIO_TypeDef *SYNC_PORT;
static uint16_t SYNC_PIN;

// 初始化DAC8568 (参考数据手册第4页引脚定义)
void DAC8568_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *sync_port, uint16_t sync_pin)
{
    hspi_dac = hspi;
    SYNC_PORT = sync_port;
    SYNC_PIN = sync_pin;

    // 初始化SYNC引脚为高电平(空闲状态)
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);

    // 可选: 执行软件复位(参考数据手册第39页表6)
     DAC8568_SoftwareReset();

    // 可选: 启用内部参考(参考数据手册第44页表7)
    // DAC8568_EnableInternalRef(REF_ENABLE);
}

// 写入数据到输入寄存器(不更新输出) (参考数据手册第35页表4)
void DAC8568_Write(uint8_t channel, uint16_t data)
{
    uint8_t txData[4]; // 使用4字节(32位)传输

    // 32位帧格式:
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(CMD_WRITE_INPUT_REG = 0000)
    // [23:20] 地址位(通道选择)
    // [19:4]  数据位(16位)
    // [3:0]   特征位(0000)

    // 第一个字节:
    // 7-4位: 前缀位(0000)
    // 3-0位: 命令位(0000)
    txData[0] = 0b00000000 | (CMD_WRITE_INPUT_REG & 0b00001111);

    // 第二个字节:
    // 7-4位: 地址位(通道选择)
    // 3-0位: 数据高4位(19-16位)
    txData[1] = ((channel & 0b00001111) << 4) | ((data >> 12) & 0b00001111);

    // 第三个字节:
    // 7-0位: 数据中间8位(15-8位)
    txData[2] = (data >> 4) & 0xFF;

    // 第四个字节:
    // 7-4位: 数据低4位(7-4位)
    // 3-0位: 特征位(0000)
    txData[3] = ((data & 0b00001111) << 4) | 0b00000000;

    // SPI传输
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY); // 传输4字节
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}

// 更新DAC寄存器(输出新值) (参考数据手册第36页表4)
void DAC8568_Update(uint8_t channel)
{
    uint8_t txData[4]; // 使用4字节(32位)传输

    // 32位帧格式:
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(CMD_UPDATE_DAC_REG = 0001)
    // [23:20] 地址位(通道选择)
    // [19:4]  数据位(均为0)
    // [3:0]   特征位(0000)

    // 第一个字节:
    // 7-4位: 前缀位(0000)
    // 3-0位: 命令位(0001)
    txData[0] = 0b00000000 | (CMD_UPDATE_DAC_REG & 0b00001111);

    // 第二个字节:
    // 7-4位: 地址位(通道选择)
    // 3-0位: 数据高4位(均为0)
    txData[1] = ((channel & 0b00001111) << 4) | 0b00000000;

    // 第三个字节和第四个字节: 均为0
    txData[2] = 0;
    txData[3] = 0;

    // SPI传输
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY); // 传输4字节
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}

// 写入并立即更新DAC输出 (参考数据手册第37页表4)
void DAC8568_WriteAndUpdate(uint8_t channel, uint16_t data)
{
    uint8_t txData[4]; // 使用4字节(32位)传输

    // 32位帧格式 (参考数据手册第37页表4):
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(CMD_WRITE_INPUT_UPDATE_ONE = 0011)
    // [23:20] 地址位(通道选择)
    // [19:4]  数据位(16位)
    // [3:0]   特征位(0000)

    // 第一个字节:
    // 7-4位: 前缀位(0000)
    // 3-0位: 命令位(0011)
    txData[0] = 0b00000000 | (CMD_WRITE_INPUT_UPDATE_ONE & 0b00001111);

    // 第二个字节:
    // 7-4位: 地址位(通道选择)
    // 3-0位: 数据高4位(19-16位)
    txData[1] = ((channel & 0b00001111) << 4) | ((data >> 12) & 0b00001111);

    // 第三个字节:
    // 7-0位: 数据中间8位(15-8位)
    txData[2] = (data >> 4) & 0xFF;

    // 第四个字节:
    // 7-4位: 数据低4位(7-4位)
    // 3-0位: 特征位(0000)
    txData[3] = ((data & 0b00001111) << 4) | 0b00000000;

    // SPI传输 (参考数据手册第7-8页时序要求)
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY); // 传输4字节
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}

// 写入所有通道(广播模式) (参考数据手册第36页表4)
void DAC8568_WriteAllChannels(uint16_t *data)
{
    for (uint8_t ch = 0; ch < 8; ch++)
    {
        DAC8568_Write(ch, data[ch]);
    }
}

// 更新所有通道(广播模式) (参考数据手册第36页表4)
void DAC8568_UpdateAllChannels(void)
{
    uint8_t txData[4]; // 使用4字节(32位)传输

    // 32位帧格式:
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(CMD_UPDATE_DAC_REG = 0001)
    // [23:20] 地址位(BROADCAST = 1111)
    // [19:4]  数据位(均为0)
    // [3:0]   特征位(0000)

    // 第一个字节:
    // 7-4位: 前缀位(0000)
    // 3-0位: 命令位(0001)
    txData[0] = 0b00000000 | (CMD_UPDATE_DAC_REG & 0b00001111);

    // 第二个字节:
    // 7-4位: 地址位(广播地址1111)
    // 3-0位: 数据高4位(均为0)
    txData[1] = (BROADCAST << 4) | 0b00000000;

    // 第三个字节和第四个字节: 均为0
    txData[2] = 0;
    txData[3] = 0;

    // SPI传输
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY); // 传输4字节
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}

// 设置电源模式 (参考数据手册第47页表13)
void DAC8568_SetPowerMode(uint8_t channel, uint8_t mode)
{
    uint8_t txData[4]; // 使用4字节(32位)传输

    // 32位帧格式:
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(CMD_POWER_DOWN = 0100)
    // [23:20] 地址位(通道选择)
    // [19:10] 均为0
    // [9:8]   PD1和PD0电源模式位
    // [7:0]   均为0 (包括特征位)

    // 第一个字节:
    // 7-4位: 前缀位(0000)
    // 3-0位: 命令位(0100)
    txData[0] = 0b00000000 | (CMD_POWER_DOWN & 0b00001111);

    // 第二个字节:
    // 7-4位: 地址位(通道选择)
    // 3-0位: 数据高4位(均为0)
    txData[1] = ((channel & 0b00001111) << 4) | 0b00000000;

    // 第三个字节:
    // 7-6位: 均为0
    // 5-4位: PD1和PD0电源模式位
    // 3-0位: 均为0
    txData[2] = ((mode & 0b00000011) << 4) | 0b00000000;

    // 第四个字节: 均为0
    txData[3] = 0;

    // SPI传输
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY); // 传输4字节
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}

// 启用/禁用内部参考 (参考数据手册第44-45页表7-11)
void DAC8568_EnableInternalRef(uint8_t mode)
{
    uint8_t txData[4]; // 使用4字节(32位)传输

    // 静态模式 (参考数据手册第44页表7)
    // 32位帧格式:
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(CMD_INTERNAL_REF = 1000)
    // [23:4]  均为0
    // [3:0]   F0=禁用/启用内部参考位(特征位)

    // 第一个字节:
    // 7-4位: 前缀位(0000)
    // 3-0位: 命令位(1000)
    txData[0] = 0b00000000 | (CMD_INTERNAL_REF & 0b00001111);

    // 第二个字节:
    // 7-4位: 地址位(0000)
    // 3-0位: 数据高4位(均为0)
    txData[1] = 0b00000000;

    // 第三个字节: 均为0
    txData[2] = 0b00000000;

    // 第四个字节: 
    // 7-1位: 均为0
    // 0位: F0=禁用/启用内部参考位
    txData[3] = (mode & 0b00000001);

    // 灵活模式需要不同的命令结构 (参考数据手册第45页表9-11)
    if (mode >= REF_FLEX_MODE_ENABLE)
    {
        // 32位帧格式:
        // [31:28] 前缀位(0000)
        // [27:24] 命令位(CMD_INTERNAL_REF = 1000)
        // [23:20] 地址位(0001 = 灵活模式)
        // [19]    1 = 灵活模式标志位
        // [18:17] 灵活模式控制位
        // [16:0]  均为0

        // 第一个字节:
        // 7-4位: 前缀位(0000)
        // 3-0位: 命令位(1000)
        txData[0] = 0b00000000 | (CMD_INTERNAL_REF & 0b00001111);

        // 第二个字节:
        // 7-4位: 地址位(0001)
        // 3位: 1 (灵活模式标志位)
        // 2-1位: 灵活模式控制位
        // 0位: 0
        txData[1] = (0b0001 << 4) | (0b1 << 3) | (((mode - REF_FLEX_MODE_ENABLE) & 0b00000011) << 1);

        // 第三个字节和第四个字节: 均为0
        txData[2] = 0;
        txData[3] = 0;
    }

    // SPI传输
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY); // 传输4字节
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}

// 设置清除代码寄存器 (参考数据手册第39页表5)
void DAC8568_SetClearCode(uint8_t mode)
{
    uint8_t txData[4]; // 使用4字节(32位)传输

    // 32位帧格式:
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(CMD_CLEAR_CODE_REG = 0101)
    // [23:4]  均为0
    // [3:2]   F1和F0位控制清除模式
    // [1:0]   均为0

    // 第一个字节:
    // 7-4位: 前缀位(0000)
    // 3-0位: 命令位(0101)
    txData[0] = 0b00000000 | (CMD_CLEAR_CODE_REG & 0b00001111);

    // 第二个字节:
    // 7-4位: 地址位(0000)
    // 3-0位: 数据高4位(均为0)
    txData[1] = 0b00000000;

    // 第三个字节: 均为0
    txData[2] = 0;

    // 第四个字节:
    // 7-4位: 均为0
    // 3-2位: F1和F0位控制清除模式
    // 1-0位: 均为0
    txData[3] = ((mode & 0b00000011) << 2);

    // SPI传输
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY); // 传输4字节
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}

// 软件复位 (参考数据手册第39页表6)
void DAC8568_SoftwareReset(void)
{
    uint8_t txData[4]; // 使用4字节(32位)传输

    // 32位帧格式:
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(CMD_SOFTWARE_RESET = 0111)
    // [23:0]  均为0

    // 第一个字节:
    // 7-4位: 前缀位(0000)
    // 3-0位: 命令位(0111)
    txData[0] = 0b00000000 | (CMD_SOFTWARE_RESET & 0b00001111);

    // 第二个、第三个和第四个字节: 均为0
    txData[1] = 0;
    txData[2] = 0;
    txData[3] = 0;

    // SPI传输
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY); // 传输4字节
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}

/**
 * @brief 直接发送32位命令到DAC8568
 * @param cmd_bits   4位命令码
 * @param addr_bits  4位地址
 * @param data_bits  16位数据
 * @param feature_bits 4位特征位
 */
void DAC8568_SendRawCommand(uint8_t cmd_bits, uint8_t addr_bits, uint16_t data_bits, uint8_t feature_bits)
{
    uint8_t txData[4]; // 使用4字节(32位)传输
    
    // 32位帧格式:
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(cmd_bits)
    // [23:20] 地址位(addr_bits)
    // [19:4]  数据位(data_bits)
    // [3:0]   特征位(feature_bits)

    // 第一个字节:
    // 7-4位: 前缀位(0000)
    // 3-0位: 命令位
    txData[0] = 0b00000000 | (cmd_bits & 0b00001111);
    
    // 第二个字节:
    // 7-4位: 地址位
    // 3-0位: 数据高4位(19-16位)
    txData[1] = ((addr_bits & 0b00001111) << 4) | ((data_bits >> 12) & 0b00001111);
    
    // 第三个字节:
    // 7-0位: 数据中间8位(15-8位)
    txData[2] = (data_bits >> 4) & 0xFF;
    
    // 第四个字节:
    // 7-4位: 数据低4位(7-4位)
    // 3-0位: 特征位
    txData[3] = ((data_bits & 0b00001111) << 4) | (feature_bits & 0b00001111);
    
    // 开始传输(拉低SYNC)
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    
    // SPI传输
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY); // 传输4字节
    
    // 结束传输(拉高SYNC)
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}

/**
 * @brief 直接发送自定义的原始数据到DAC8568
 * @param raw_data 4字节原始数据数组
 */
void DAC8568_SendRawData(uint8_t raw_data[4])
{
    // 开始传输(拉低SYNC)
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    
    // SPI传输
    HAL_SPI_Transmit(hspi_dac, raw_data, 4, HAL_MAX_DELAY); // 传输4字节
    
    // 结束传输(拉高SYNC)
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}