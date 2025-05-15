/*
 * DAC8568/DAC8168/DAC8568 驱动程序
 * 作者: 雪豹
 */
#include "dac8568.h"

static SPI_HandleTypeDef *hspi_dac; // SPI句柄指针，用于SPI通信
static GPIO_TypeDef *SYNC_PORT;     // SYNC引脚的GPIO端口指针
static uint16_t SYNC_PIN;           // SYNC引脚的引脚号

/**
 * @brief 初始化DAC8568驱动。
 * @param hspi SPI外设句柄指针。
 * @param sync_port SYNC引脚的GPIO端口。
 * @param sync_pin SYNC引脚的引脚号。
 * @note 此函数会保存SPI和SYNC引脚的配置，并将SYNC引脚初始化为高电平。
 *       默认执行软件复位，可选启用内部参考电压。
 */
void DAC8568_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *sync_port, uint16_t sync_pin)
{
    hspi_dac = hspi;       // 保存SPI句柄
    SYNC_PORT = sync_port; // 保存SYNC引脚的端口
    SYNC_PIN = sync_pin;   // 保存SYNC引脚的引脚号

    // 初始化SYNC引脚为高电平(空闲状态)
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);

    // 可选: 执行软件复位(参考数据手册第39页表6)
    DAC8568_SoftwareReset(); // 执行软件复位，确保DAC上电后处于已知的默认状态

    // 可选: 启用内部参考(参考数据手册第44页表7)
    // DAC8568_EnableStaticInternalRef(); // 如果需要使用内部参考电压，取消此行注释
}

/**
 * @brief 向DAC指定通道的输入寄存器写入数据。
 * @param channel 目标DAC通道 (CHANNEL_A 到 CHANNEL_H, 或 BROADCAST)。
 * @param data 要写入的16位数据。
 * @note 此操作仅更新输入寄存器，不会立即更新DAC的模拟输出。
 *       参考数据手册第35页表4。
 */
void DAC8568_Write(uint8_t channel, uint16_t data)
{
    uint8_t txData[4]; // 定义SPI传输的4字节数据缓冲区

    // 32位帧格式:
    // [31:28] 前缀位(0000) - 固定为0
    // [27:24] 命令位(CMD_WRITE_INPUT_REG = 0000) - 写入输入寄存器命令
    // [23:20] 地址位(channel) - 选择目标DAC通道
    // [19:4]  数据位(data) - 16位DAC数据
    // [3:0]   特征位(0000) - 此命令中不使用，固定为0

    // 第一个字节 (DB31-DB24): 包含前缀位和命令位
    // 7-4位 (DB31-DB28): 前缀位 (0000)
    // 3-0位 (DB27-DB24): 命令位 (CMD_WRITE_INPUT_REG = 0000)
    txData[0] = 0b00000000 | (CMD_WRITE_INPUT_REG & 0b00001111); // 构造第一个字节

    // 第二个字节 (DB23-DB16): 包含地址位和数据位的高4位
    // 7-4位 (DB23-DB20): 地址位 (通道选择)
    // 3-0位 (DB19-DB16): 16位数据的最高4位 (D15-D12)
    txData[1] = ((channel & 0b00001111) << 4) | ((data >> 12) & 0b00001111); // 构造第二个字节

    // 第三个字节 (DB15-DB8): 包含数据位的中间8位
    // 7-0位 (DB15-DB8): 16位数据的中间8位 (D11-D4)
    txData[2] = (data >> 4) & 0xFF; // 构造第三个字节

    // 第四个字节 (DB7-DB0): 包含数据位的低4位和特征位
    // 7-4位 (DB7-DB4): 16位数据的最低4位 (D3-D0)
    // 3-0位 (DB3-DB0): 特征位 (0000)
    txData[3] = ((data & 0b00001111) << 4) | 0b00000000; // 构造第四个字节

    // 执行SPI传输
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET); // 拉低SYNC引脚，片选DAC，开始传输
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY);   // 通过SPI发送4个字节的数据
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);   // 拉高SYNC引脚，取消片选DAC，结束传输
}

/**
 * @brief 更新DAC指定通道的输出。
 * @param channel 目标DAC通道 (CHANNEL_A 到 CHANNEL_H, 或 BROADCAST)。
 * @note 此操作会将对应通道输入寄存器中的值加载到DAC寄存器，从而更新模拟输出。
 *       参考数据手册第36页表4。
 */
void DAC8568_Update(uint8_t channel)
{
    uint8_t txData[4]; // 定义SPI传输的4字节数据缓冲区

    // 32位帧格式:
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(CMD_UPDATE_DAC_REG = 0001) - 更新DAC寄存器命令
    // [23:20] 地址位(channel) - 选择目标DAC通道
    // [19:4]  数据位(均为0) - 此命令中数据位不使用
    // [3:0]   特征位(0000) - 此命令中不使用

    // 第一个字节 (DB31-DB24): 前缀位 + 命令位
    txData[0] = 0b00000000 | (CMD_UPDATE_DAC_REG & 0b00001111);

    // 第二个字节 (DB23-DB16): 地址位 + 数据高4位(0)
    txData[1] = ((channel & 0b00001111) << 4) | 0b00000000;

    // 第三个字节 (DB15-DB8): 数据中间8位(0)
    txData[2] = 0;
    // 第四个字节 (DB7-DB0): 数据低4位(0) + 特征位(0)
    txData[3] = 0;

    // 执行SPI传输
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}

/**
 * @brief 向DAC指定通道的输入寄存器写入数据并立即更新其模拟输出。
 * @param channel 目标DAC通道 (CHANNEL_A 到 CHANNEL_H)。
 * @param data 要写入的16位数据。
 * @note 参考数据手册第37页表4。
 */
void DAC8568_WriteAndUpdate(uint8_t channel, uint16_t data)
{
    uint8_t txData[4]; // 定义SPI传输的4字节数据缓冲区

    // 32位帧格式 (参考数据手册第37页表4):
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(CMD_WRITE_INPUT_UPDATE_ONE = 0011) - 写入输入寄存器并更新单个DAC命令
    // [23:20] 地址位(channel) - 选择目标DAC通道
    // [19:4]  数据位(data) - 16位DAC数据
    // [3:0]   特征位(0000) - 此命令中不使用

    // 第一个字节 (DB31-DB24): 前缀位 + 命令位
    txData[0] = 0b00000000 | (CMD_WRITE_INPUT_UPDATE_ONE & 0b00001111);

    // 第二个字节 (DB23-DB16): 地址位 + 数据高4位
    txData[1] = ((channel & 0b00001111) << 4) | ((data >> 12) & 0b00001111);

    // 第三个字节 (DB15-DB8): 数据中间8位
    txData[2] = (data >> 4) & 0xFF;

    // 第四个字节 (DB7-DB0): 数据低4位 + 特征位
    txData[3] = ((data & 0b00001111) << 4) | 0b00000000;

    // 执行SPI传输 (参考数据手册第7-8页时序要求)
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}

/**
 * @brief 将数据数组中的值分别写入所有8个DAC通道的输入寄存器。
 * @param data_array 包含8个uint16_t类型数据的数组指针，分别对应通道A到H。
 * @note 此函数通过循环调用 DAC8568_Write 实现。
 *       参考数据手册第36页表4。
 */
void DAC8568_WriteAllChannels(uint16_t *data_array) // 参数为包含8个通道数据的数组指针
{
    for (uint8_t ch = 0; ch < 8; ch++) // 遍历0到7，代表通道A到H
    {
        DAC8568_Write(ch, data_array[ch]); // 为当前通道 ch 写入数组中对应的数据
    }
}

/**
 * @brief 更新所有DAC通道的模拟输出。
 * @note 此函数使用CMD_UPDATE_DAC_REG命令和BROADCAST地址。
 *       参考数据手册第36页表4。
 */
void DAC8568_UpdateAllChannels(void)
{
    uint8_t txData[4]; // 定义SPI传输的4字节数据缓冲区

    // 32位帧格式:
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(CMD_UPDATE_DAC_REG = 0001)
    // [23:20] 地址位(BROADCAST = 1111) - 广播地址，操作所有通道
    // [19:4]  数据位(均为0)
    // [3:0]   特征位(0000)

    // 第一个字节 (DB31-DB24): 前缀位 + 命令位
    txData[0] = 0b00000000 | (CMD_UPDATE_DAC_REG & 0b00001111);

    // 第二个字节 (DB23-DB16): 广播地址 + 数据高4位(0)
    txData[1] = (BROADCAST << 4) | 0b00000000; // 使用 BROADCAST 地址

    // 第三个字节 (DB15-DB8): 数据中间8位(0)
    txData[2] = 0;
    // 第四个字节 (DB7-DB0): 数据低4位(0) + 特征位(0)
    txData[3] = 0;

    // 执行SPI传输
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}

/**
 * @brief 设置指定DAC通道或所有通道的电源模式。
 * @param channel 目标通道 (CHANNEL_A 到 CHANNEL_H) 或 BROADCAST。
 * @param mode 电源模式 (POWER_UP, POWER_DOWN_1K, POWER_DOWN_100K, POWER_DOWN_HIZ)。
 * @note 参考数据手册第47页表13。
 */
void DAC8568_SetPowerMode(uint8_t channel, uint8_t mode)
{
    uint8_t txData[4]; // 定义SPI传输的4字节数据缓冲区

    // 32位帧格式:
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(CMD_POWER_DOWN = 0100) - 电源控制命令
    // [23:20] 地址位(channel) - 选择目标通道或广播
    // [19:10] 均为0 (不关心)
    // [9:8]   PD1和PD0电源模式位 (位于整个SPI帧的DB9, DB8)
    // [7:0]   均为0 (不关心, 包括特征位)

    // 第一个字节 (DB31-DB24): 前缀位 + 命令位
    txData[0] = 0b00000000 | (CMD_POWER_DOWN & 0b00001111);

    // 第二个字节 (DB23-DB16): 地址位 + 数据高4位(此命令中为0)
    txData[1] = ((channel & 0b00001111) << 4) | 0b00000000;

    // 第三个字节 (DB15-DB8): 包含电源模式位 PD1(DB9) 和 PD0(DB8)
    // (mode & 0b00000011) 提取 mode 参数中的 PD1(bit1) 和 PD0(bit0)。
    // 这些位对应SPI帧的DB9和DB8，因此直接放入 txData[2] 的最低两位。
    // txData[2] 的高6位 (DB15-DB10) 为 不关心 (设为0)。
    txData[2] = (mode & 0b00000011); // PD1位于bit1, PD0位于bit0 of txData[2] (对应帧的D9, D8)

    // 第四个字节 (DB7-DB0): 均为0 (不关心)
    txData[3] = 0;

    // 执行SPI传输
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}

/**
 * @brief 启用静态内部参考电压（2.5V）。
 * @note 参考数据手册第44页表7。命令 CMD_INTERNAL_REF, 地址 0b0000, 特征位 REF_ENABLE。内部参考电压为2.5V，但是在C/D等级中内部会有一个2倍增益。见数据手册31页8.2.1
 */
void DAC8568_EnableStaticInternalRef(void)
{
    DAC8568_SendRawCommand(CMD_INTERNAL_REF, 0b0000, 0x0000, REF_ENABLE);
}

/**
 * @brief 禁用静态内部参考电压（2.5V）。
 * @note 参考数据手册第44页表7。命令 CMD_INTERNAL_REF, 地址 0b0000, 特征位 REF_DISABLE。内部参考电压为2.5V，但是在C/D等级中内部会有一个2倍增益。见数据手册31页8.2.1
 */
void DAC8568_DisableStaticInternalRef(void)
{
    DAC8568_SendRawCommand(CMD_INTERNAL_REF, 0b0000, 0x0000, REF_DISABLE);
}

/**
 * @brief 启用内部参考的灵活模式 (Flex Mode)。
 * @note 参考数据手册第45页表9。命令 CMD_INTERNAL_REF, 地址 0b0001, 数据位 D13=1, 特征位 0b0000。
 */
void DAC8568_EnableFlexMode(void)
{
    // 数据位 D13 (对应整个16位数据字段的 bit 13) 设置为1
    DAC8568_SendRawCommand(CMD_INTERNAL_REF, 0b0001, (1 << 13), 0b0000);
}

/**
 * @brief 禁用内部参考的灵活模式 (Flex Mode)。
 * @note 参考数据手册第45页表9。命令 CMD_INTERNAL_REF, 地址 0b0001, 数据位 D13=0, 特征位 0b0000。
 *       禁用灵活模式后，内部参考的行为由静态模式控制位决定。
 */
void DAC8568_DisableFlexMode(void)
{
    // 数据位 D13 设置为0
    DAC8568_SendRawCommand(CMD_INTERNAL_REF, 0b0001, 0x0000, 0b0000);
}

/**
 * @brief 在灵活模式下，设置内部参考是否始终开启。
 * @param enable 1 表示始终开启, 0 表示根据需要自动开启/关闭 (默认行为)。
 * @note 参考数据手册第45页表10。命令 CMD_INTERNAL_REF, 地址 0b0001, 数据位 D15 控制, 特征位 0b0000。
 */
void DAC8568_SetFlexModeRefAlwaysOn(uint8_t enable)
{
    uint16_t data_bits = enable ? (1 << 15) : 0; // D15 控制
    DAC8568_SendRawCommand(CMD_INTERNAL_REF, 0b0001, data_bits, 0b0000);
}

/**
 * @brief 在灵活模式下，设置内部参考是否始终关闭。
 * @param enable 1 表示始终关闭, 0 表示根据需要自动开启/关闭。
 * @note 参考数据手册第45页表11。命令 CMD_INTERNAL_REF, 地址 0b0001, 数据位 D14 控制, 特征位 0b0000。
 */
void DAC8568_SetFlexModeRefAlwaysOff(uint8_t enable)
{
    uint16_t data_bits = enable ? (1 << 14) : 0; // D14 控制
    DAC8568_SendRawCommand(CMD_INTERNAL_REF, 0b0001, data_bits, 0b0000);
}

/**
 * @brief 设置DAC清零时的行为。
 * @param mode 清除代码模式 (CLEAR_CODE_ZERO_SCALE, CLEAR_CODE_MID_SCALE, CLEAR_CODE_FULL_SCALE, CLEAR_CODE_NO_OPERATION)。
 * @note 参考数据手册第39页表5。
 */
void DAC8568_SetClearCode(uint8_t mode)
{
    uint8_t txData[4]; // 定义SPI传输的4字节数据缓冲区

    // 32位帧格式:
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(CMD_CLEAR_CODE_REG = 0101) - 清除代码寄存器命令
    // [23:4]  均为0 (不关心)
    // [3:2]   特征位 F1, F0 (位于整个SPI帧的DB3, DB2) 控制清除模式
    // [1:0]   均为0 (不关心)

    // 第一个字节 (DB31-DB24): 前缀位 + 命令位
    txData[0] = 0b00000000 | (CMD_CLEAR_CODE_REG & 0b00001111);

    // 第二个字节 (DB23-DB16): 地址位(不关心, 设为0) + 数据高位(不关心, 设为0)
    txData[1] = 0b00000000;

    // 第三个字节 (DB15-DB8): 数据中间位(不关心, 设为0)
    txData[2] = 0;

    // 第四个字节 (DB7-DB0): 包含特征位 F1, F0
    // (mode & 0b00000011) 提取 mode 参数的最低两位作为 F1(bit1)F0(bit0)。
    // 左移2位，将它们放置到SPI帧的DB3和DB2位置。
    txData[3] = (mode & 0b00000011) << 2; // F1(DB3), F0(DB2) 控制清除模式

    // 执行SPI传输
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
}

/**
 * @brief 执行软件复位。
 * @note 将DAC所有寄存器恢复到上电默认状态。
 *       参考数据手册第39页表6。
 *       软件复位后建议等待一小段时间 (例如1ms)。
 */
void DAC8568_SoftwareReset(void)
{
    uint8_t txData[4]; // 定义SPI传输的4字节数据缓冲区

    // 32位帧格式:
    // [31:28] 前缀位(0000)
    // [27:24] 命令位(CMD_SOFTWARE_RESET = 0111) - 软件复位命令
    // [23:0]  均为0 (不关心)

    // 第一个字节 (DB31-DB24): 前缀位 + 命令位
    txData[0] = 0b00000000 | (CMD_SOFTWARE_RESET & 0b00001111);
    // 其余字节 (DB23-DB0) 均为 不关心，设为0
    txData[1] = 0;
    txData[2] = 0;
    txData[3] = 0;

    // 执行SPI传输
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(hspi_dac, txData, 4, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SYNC_PORT, SYNC_PIN, GPIO_PIN_SET);
    HAL_Delay(1); // 根据数据手册建议，软件复位后等待一小段时间 (1ms为保守值)
}

/**
 * @brief 直接发送一个完整的32位命令帧到DAC8568。
 * @param cmd_bits 4位命令位 (DB27-DB24)。
 * @param addr_bits 4位地址位 (DB23-DB20)。
 * @param data_bits 16位数据位 (DB19-DB4)。
 * @param feature_bits 4位特征位 (DB3-DB0)。
 * @note 前缀位 (DB31-DB28) 固定为0b0000。
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
 * @brief 直接发送自定义的4字节原始数据到DAC8568。
 * @param raw_data 指向包含4字节数据的数组。
 * @note 此函数用于发送预先构建好的完整SPI帧。
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