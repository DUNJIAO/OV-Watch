#include "WDOG.h"
#include "main.h"

//WDOG_EN
#define WDOG_EN_PORT	GPIOB
#define WDOG_EN_PIN		GPIO_PIN_1
//WDI
#define WDI_PORT	GPIOB
#define WDI_PIN		GPIO_PIN_2

/**
  * @brief  配置看门狗的两个引脚 PB1(EN)、PB2(WDI)
  * @retval None
  * @note   EN 配成推挽输出，速度 LOW；
  *         WDI 复用同一个 GPIO_InitStructure，所以 Mode/Pull 没变，
  *         只把 Speed 改成 HIGH —— 这是翻转信号需要快边沿
  */
void WDOG_Port_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStructure.Pin = WDOG_EN_PIN;
  GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Pull = GPIO_PULLUP;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(WDOG_EN_PORT, &GPIO_InitStructure);
  GPIO_InitStructure.Pin = WDI_PIN;
  GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(WDI_PORT, &GPIO_InitStructure);
}

/**
  * @brief  使能外部看门狗：EN 脚输出低电平
  * @retval None
  * @note   这颗芯片是低电平使能，所以 RESET 才是"打开"
  */
void WDOG_Enable(void)
{
  HAL_GPIO_WritePin(WDOG_EN_PORT,WDOG_EN_PIN,GPIO_PIN_RESET);
}

/**
  * @brief  关闭外部看门狗：EN 脚输出高电平
  * @retval None
  * @note   STOP 睡眠前调用，醒来喂一次狗后再重新使能
  */
void WDOG_Disnable(void)
{
  HAL_GPIO_WritePin(WDOG_EN_PORT,WDOG_EN_PIN,GPIO_PIN_SET);
}

/**
  * @brief  喂狗：翻转 WDI 脚电平
  * @retval None
  * @note   不关心当前是高还是低，翻转一次就算一次有效喂狗
  */
void WDOG_Feed(void)
{
  HAL_GPIO_TogglePin(WDI_PORT,WDI_PIN);
}
