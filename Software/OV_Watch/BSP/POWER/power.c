#include "power.h"
#include "adc.h"
#include "delay.h"

/* 电池内阻，单位 Ω；充电时电流会在它上面产生压降 */
#define INTERNAL_RES 0.128
/* 充电电流，单位 A */
#define CHARGING_CUR 1

/**
  * @brief  配置 POWER、CHARGE 两个引脚，并打开 EXTI2 中断
  * @retval None
  * @note   POWER 先输出低电平再配成推挽输出；
  *         CHARGE 配成双边沿中断（插入充电器、拔掉都会触发）。
  *         BAT_CHECK 引脚不在这里配置，由 adc.c 负责
  */
void Power_Pins_Init()
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(POWER_PORT, POWER_PIN, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA3 */
  GPIO_InitStruct.Pin = POWER_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(POWER_PORT, &GPIO_InitStruct);

  /*Configure GPIO pin : PA2 */
  GPIO_InitStruct.Pin = CHARGE_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(CHARGE_PORT, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

}

/**
  * @brief  拉高 POWER 脚，保持整机供电
  * @retval None
  */
void Power_Enable()
{
	HAL_GPIO_WritePin(POWER_PORT,POWER_PIN,GPIO_PIN_SET);
}

/**
  * @brief  拉低 POWER 脚，断电关机
  * @retval None
  */
void Power_DisEnable()
{
	HAL_GPIO_WritePin(POWER_PORT,POWER_PIN,GPIO_PIN_RESET);
}

/**
  * @brief  判断是否在充电
  * @retval 1：正在充电；0：未充电
  */
uint8_t ChargeCheck()//1:charging
{
	return HAL_GPIO_ReadPin(CHARGE_PORT,CHARGE_PIN);
}

/**
  * @brief  读一次电池电压（不做平均）
  * @retval 电压，单位 V
  * @note   dat * 2 是因为电池电压经过了一半的分压；4096 是 12 位 ADC 的最大刻度
  */
float BatCheck()
{
	uint16_t dat;
	float BatVoltage;
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1,5);
	dat = HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
	BatVoltage = dat *2 *3.3 /4096;
	return BatVoltage;
}

/**
  * @brief  连续采样 8 次求平均后的电池电压
  * @retval 电压，单位 V
  * @note   每次采样之间 delay_ms(1)，整函数约 8 ms，属于"调用一次比较贵"的函数
  */
float BatCheck_8times()
{
	uint32_t dat=0;
	uint8_t i;
	float BatVoltage;
	for(i=0;i<8;i++)
	{
		HAL_ADC_Start(&hadc1);
		HAL_ADC_PollForConversion(&hadc1,5);
		dat += HAL_ADC_GetValue(&hadc1);
		HAL_ADC_Stop(&hadc1);
		delay_ms(1);
	}
	dat = dat>>3;
	BatVoltage = dat *2 *3.3 /4096;
	return BatVoltage;
}

/**
  * @brief  由电压换算成电量百分比
  * @retval 电量百分比，共 11 档：100/90/80/70/60/50/40/30/20/10/5
  * @note   充电时先减去内阻压降 INTERNAL_RES × CHARGING_CUR。
  *         power 没有初始化，电压低于 3.45 V 时 if 链全不命中，返回未定义值
  */
uint8_t PowerCalculate()
{
	uint8_t power;
	float voltage;
	voltage = BatCheck_8times();

	if(ChargeCheck())
	{voltage -= INTERNAL_RES * CHARGING_CUR;}

	if((voltage >= 4.2))
	{power = 100;}
	else if(voltage >= 4.06 && voltage <4.2)
	{power = 90;}
	else if(voltage >= 3.98 && voltage <4.06)
	{power = 80;}
	else if(voltage >= 3.92 && voltage <3.98)
	{power = 70;}
	else if(voltage >= 3.87 && voltage <3.92)
	{power = 60;}
	else if(voltage >= 3.82 && voltage <3.87)
	{power = 50;}
	else if(voltage >= 3.79 && voltage <3.82)
	{power = 40;}
	else if(voltage >= 3.77 && voltage <3.79)
	{power = 30;}
	else if(voltage >= 3.74 && voltage <3.77)
	{power = 20;}
	else if(voltage >= 3.68 && voltage <3.74)
	{power = 10;}
	else if(voltage >= 3.45 && voltage <3.68)
	{power = 5;}
	return power;
}

/**
  * @brief  初始化电源引脚并打开供电自锁
  * @retval None
  */
void Power_Init(void)
{
	Power_Pins_Init();
	Power_Enable();
}
