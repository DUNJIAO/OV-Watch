#include "KT6328.h"

/**
  * @brief  配置蓝牙模块的使能脚 PA8
  * @retval None
  * @note   先把 PA8 写成低电平，再配成推挽输出，
  *         这样配置完成的那一刻引脚就是"关闭"状态，模块不会被误启动
  */
void KT6328_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}


/**
  * @brief  打开蓝牙模块：PA8 输出高电平
  * @retval None
  */
void KT6328_Enable(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
}


/**
  * @brief  关闭蓝牙模块：PA8 输出低电平
  * @retval None
  * @note   关掉后模块断电，重新打开需要等它自己重新启动
  */
void KT6328_Disable(void)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
}

