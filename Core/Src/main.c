/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/*
 * DAC8568/DAC8168/DAC8568 驱动程序
 * 作者: 雪豹
 */
/*
 * 注释在DAC8568.h中
 * SPI 外设配置说明 (针对 DAC8568)
 * ----------------------------------------------------------------
 * 参数要求（参考数据手册第7-8页时序图和第6页电气特性）：
 * 1. 模式: 全双工主模式 (DAC8568 为从设备)
 * 2. 数据大小: 8位 (需手动组合为24/32位帧)
 * 3. 时钟极性 (CPOL): 高电平空闲 (CPOL=high)
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

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dac8568.h" // 包含DAC8568的头文件
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  // 初始化DAC8568 (SYNC连接到PA4)
  DAC8568_Init(&hspi1, SYNC_GPIO_Port, SYNC_Pin);
  HAL_Delay(10);
  DAC8568_EnableStaticInternalRef(); // 启用静态内部参考(2.5V)
  // DAC8568_DisableStaticInternalRef(); // 禁用静态内部参考(2.5V)

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    for (int i = 65535; i >= 0; i = i - 4096) // 从65535到0，步长4096
    {
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);     // 翻转LED引脚的状态
      DAC8568_WriteAndUpdate(BROADCAST, (uint16_t)i); // 写入并更新所有通道的值 (强制类型转换为uint16_t)
      // DAC8568_WriteAndUpdate(CHANNEL_A, (uint16_t)i); // 写入并更新通道A的值 (强制类型转换为uint16_t)
      // DAC8568_WriteAndUpdate(CHANNEL_B, (uint16_t)i); // 写入并更新通道B的值 (强制类型转换为uint16_t)
      float voltage = 2.5 * 2 * i / 65536;         // 计算电压值 (假设Vref=2.5V，16位分辨率)
      HAL_Delay(2000);                             // 稍微缩短延时以便观察变化，可根据需要调整
    }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
