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
4. 或使用PlatformIO: `pio run -t upload`

## 示例代码
```c
// 初始化DAC8568 (SYNC连接到PA4)
DAC8568_Init(&hspi1, SYNC_GPIO_Port, SYNC_Pin);
DAC8568_EnableStaticInternalRef(); // 启用内部参考电压(2.5V)

// 写入并更新特定通道
DAC8568_WriteAndUpdate(CHANNEL_A, 32768); // 输出约1.25V (中间电平)

// 更新所有通道
DAC8568_UpdateAllChannels();
```

## 许可证
MIT License

## 版本信息
- 版本: 1.5
- 更新日期: 2025年5月15日
- 作者: 雪豹
