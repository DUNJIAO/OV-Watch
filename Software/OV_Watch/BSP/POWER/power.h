#ifndef _POWER_H_
#define _POWER_H_

#include "stm32f4xx_hal.h"

/*
 * 电源管理：三个引脚各管一件事
 *   PA1 BAT_CHECK 电池电压，走 ADC1（引脚配置在 adc.c 里，不在本文件）
 *   PA2 CHARGE    充电检测，双边沿中断
 *   PA3 POWER     电源自锁，拉高维持供电，写 0 等于关机
 */

#define BAT_CHECK_PORT	GPIOA
#define BAT_CHECK_PIN		GPIO_PIN_1

#define CHARGE_PORT			GPIOA
#define CHARGE_PIN			GPIO_PIN_2

#define POWER_PORT			GPIOA
#define POWER_PIN				GPIO_PIN_3

/**
  * @brief  配置 POWER、CHARGE 两个引脚，并打开 EXTI2 中断
  * @retval None
  * @note   BAT_CHECK 引脚不在这里配置，由 adc.c 的 ADC 初始化负责
  */
void Power_Pins_Init(void);

/**
  * @brief  拉高 POWER 脚，保持整机供电
  * @retval None
  */
void Power_Enable(void);

/**
  * @brief  拉低 POWER 脚，断电关机
  * @retval None
  */
void Power_DisEnable(void);

/**
  * @brief  读一次电池电压（不做平均）
  * @retval 电压，单位 V
  */
float BatCheck(void);

/**
  * @brief  连续采样 8 次求平均后的电池电压
  * @retval 电压，单位 V
  * @note   每次采样之间 delay_ms(1)，整函数约 8 ms，属于"调用一次比较贵"的函数
  */
float BatCheck_8times(void);

/**
  * @brief  判断是否在充电
  * @retval 1：正在充电；0：未充电
  */
uint8_t ChargeCheck(void);

/**
  * @brief  由电压换算成电量百分比
  * @retval 电量百分比，按电压表分成 100/90/80/70/60/50/40/30/20/10/5 共 11 档
  * @note   充电时会先减去内阻压降 INTERNAL_RES × CHARGING_CUR。
  *         power 变量没有初始化，电压低于 3.45 V 时返回未定义值
  */
uint8_t PowerCalculate(void);

/**
  * @brief  初始化电源引脚并打开供电自锁
  * @retval None
  */
void Power_Init(void);

#endif
