# DAC8568-STM32F103C8

## 项目简介
这是一个使用STM32F103C8控制器驱动TI DAC8568高精度数模转换器的开源项目。DAC8568是一款16位8通道DAC，通过SPI接口控制，适合高精度信号输出和波形生成应用。

## 主要特点
- 支持DAC8568/DAC8168/DAC7568系列全部功能
- 完整的SPI通信驱动，支持所有通道单独或广播操作
- 支持多种电源管理模式（正常/1kΩ下拉/100kΩ下拉/高阻态）
- 灵活的内部参考电压控制（2.5V参考源）
- 详细的中文注释和文档

## 硬件要求
- STM32F103C8微控制器（或兼容型号）
- DAC8568/DAC8168/DAC7568系列DAC芯片
- 基本接线：
  - MOSI -> DAC DIN
  - SCK -> DAC SCLK
  - GPIO -> DAC SYNC（软件片选）
  - VDD -> 3.3V 电源（带去耦电容）
  - GND -> 公共地

## 软件要求
- STM32CubeIDE (推荐用于开发)
- 或Visual Studio Code + PlatformIO插件

## 快速开始
1. 克隆本仓库
2. 使用STM32CubeIDE打开项目
3. 编译并烧录到您的STM32设备
4. 或使用PlatformIO

## CubeMX配置说明
在STM32CubeMX中为DAC8568配置SPI接口时，请按照以下步骤操作：

1. **SPI配置**:
   - 进入 `Connectivity` → 选择 `SPIx` (例如SPI1)
   - 设置模式 = `Transmit Only Master`或者`Full-Duplex Master`
   - 在Parameter Settings中:
     - Data Size = `8 Bits`
     - First Bit = `MSB First`
     - Clock Polarity (CPOL) = `High` (重要!)
     - Clock Phase (CPHA) = `1 Edge`
     - Baud Rate Prescaler = 适当分频值 (确保SCLK ≤ 20MHz)
     - 其他参数可保持默认

2. **SYNC引脚配置**:
   - 在 `GPIO Settings` 中选择一个空闲引脚作为SYNC信号
   - 将其配置为 `GPIO_Output`
   - 初始电平设为 `High`
   - 例如: PA4设为输出模式，标签为"SYNC"

3. **时钟配置**:
   - 根据您的应用需求配置合适的系统时钟
   - 为确保SPI通信稳定，建议使用8MHz或更高的时钟频率

4. **代码生成**:
   - 勾选 `Generate peripheral initialization as a pair of '.c/.h' files per peripheral`
   - 点击 `Generate Code` 生成项目

## 移植指南
要将DAC8568驱动移植到其他STM32平台，请按照以下步骤操作：

1. **复制核心文件**:
   - 将 `DAC8568.c` 和 `DAC8568.h` 文件复制到您的项目中
   - 确保包含相关的HAL头文件 (对大多数STM32平台是通用的)

2. **配置SPI总线**:
   - 使用CubeMX为目标平台配置SPI外设
   - 确保SPI配置符合上述"CubeMX配置说明"中的参数

3. **调整GPIO配置**:
   - 为SYNC引脚配置合适的GPIO端口和引脚,推荐引脚上右键使用pin label命名为SYNC
   - 确保在初始化代码中使用正确的GPIO句柄

4. **修改时钟频率** (如果需要):
   - 检查并调整SPI分频系数，以确保SPI时钟不超过50MHz
   - 对于更高速的STM32型号，可能需要更大的分频值

5. **集成到项目**:
   - 在您的主程序中包含 `DAC8568.h`
   - 调用初始化函数 `DAC8568_Init(&hspi_handle, sync_port, sync_pin)`
   - 使用API函数控制DAC输出


## 示例代码
```c
// 初始化DAC8568 (SYNC连接到PA4)
DAC8568_Init(&hspi1, SYNC_GPIO_Port, SYNC_Pin);
DAC8568_EnableStaticInternalRef(); // 启用内部参考电压(2.5V)
// DAC8568_DisableStaticInternalRef(); // 禁用静态内部参考(2.5V)

// 写入并更新特定通道
DAC8568_WriteAndUpdate(CHANNEL_A, 32768); // 输出约1.25V (中间电平)
DAC8568_WriteAndUpdate(BROADCAST, (uint16_t)i); // 写入并更新所有通道的值 (强制类型转换为uint16_t)

```

## 许可证
MIT License

## 版本信息
- 版本: 1.5
- 更新日期: 2025年5月15日
- 作者: 雪豹
