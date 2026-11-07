/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "dma.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/*
 * ============================================================================
 * 一、启动流程总览（本文件只负责把系统跑起来，不管业务）
 *   1. SCB->VTOR = 0x0000C000   把中断向量表搬到 0xC000（工程带 Bootloader）
 *   2. HAL_Init()               初始化 HAL：NVIC 分组、1 ms 时基、Flash 预取
 *   3. SystemClock_Config()     HSI 16 MHz -> PLL -> SYSCLK 100 MHz
 *   4. __enable_irq()           打开全局中断
 *   5. MX_xxx_Init()            逐个外设初始化，函数体在各自的 .c 里
 *   6. osKernelInitialize()     初始化 FreeRTOS 内核（还没开始调度）
 *   7. MX_FREERTOS_Init()       里面调用 User_Tasks_Init()，建队列和任务
 *   8. osKernelStart()          开始调度，此函数【永不返回】
 *
 *   第 6、7、8 步顺序不能换：先 Start 的话，后面的建任务代码根本执行不到。
 *
 * ============================================================================
 * 二、main()  --  The application entry point.
 *   从复位到进入调度器，全程只执行一次；osKernelStart 之后由任务接管，
 *   函数末尾那个 while(1) 是死代码，永远不会执行。
 *
 *   HAL_Init() 内部其实只做三件事：
 *     HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4)  4 位全部给抢占优先级
 *     HAL_InitTick()                                      1 ms 时基
 *     HAL_MspInit()                                       Flash 预取、电源配置
 *   优先级分组必须在这里定下来，后面所有 HAL_NVIC_SetPriority() 的第二个参数
 *   都是"抢占优先级"，范围 0~15。工程里出现的 5、0 就是这个含义。
 *
 * ============================================================================
 * 三、SystemClock_Config()  --  System Clock Configuration
 *   时钟树：HSI 16 MHz / PLLM 8 = 2 MHz
 *                 * PLLN 100  = 200 MHz
 *                 / PLLP 2    = 100 MHz  <- SYSCLK
 *   AHB 不分频 = 100 MHz，APB1 二分频 = 50 MHz，APB2 不分频 = 100 MHz
 *   Flash 等待周期 FLASH_LATENCY_3，100 MHz 下必须给 3 个等待周期
 *   同时打开 LSE（32.768 kHz），给 RTC 用
 *
 *   注意：进 STOP 模式会把时钟切回内部 HSI 16 MHz，
 *   所以 user_RunModeTasks.c 里醒来后必须再调一次本函数，否则按 16 MHz 跑。
 *
 * ============================================================================
 * 四、HAL_TIM_PeriodElapsedCallback()  --  TIM1 周期中断回调
 *   本工程把 HAL 的时基从 SysTick 换成了 TIM1，因为 SysTick 已经被 FreeRTOS 占用。
 *   这里每进一次中断就把 uwTick 加 1，HAL_Delay 和 HAL_GetTick 才有时间可读。
 *   配置代码在 stm32f4xx_hal_timebase_tim.c。
 *   结果是两套时基并存：HAL_Delay() 走 TIM1，osDelay() 走 SysTick，别混用。
 *
 * ============================================================================
 * 五、Error_Handler()  --  初始化失败时进入
 *   一旦进来就先关全局中断再死循环，现场停在 while(1) 里；
 *   用调试器暂停就能看到调用栈，据此判断是哪个初始化失败。
 *   本工程里 HAL_RCC_OscConfig、HAL_RCC_ClockConfig、HAL_RTCEx_SetWakeUpTimer_IT
 *   失败都会走到这里。
 *
 * ============================================================================
 * 六、关于重新生成
 *   本文件由 CubeMX 生成。只有 USER CODE BEGIN/END 之间的内容会被保存，
 *   其余位置（包括函数上方的 @brief 注释）重新生成时会被覆盖。
 *   所以上面这五段说明全部集中放在本区块内，保证不会丢。
 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	//this must set same as keil setting
	SCB->VTOR = 0x0000C000U;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	__enable_irq();
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_RTC_Init();
  MX_SPI1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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

#ifdef  USE_FULL_ASSERT
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
