#ifndef __WDOG_H__
#define __WDOG_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * OWDG = 外部看门狗芯片（不是 STM32 内部的 IWDG）
 * EN  = PB1，低电平使能看门狗，高电平关闭
 * WDI = PB2，喂狗脚：电平翻转一次就算喂一次，超时不翻转芯片就复位整机
 * 喂狗周期由 WDOGFeedTask 控制，每 100 ms 一次
 */

/**
  * @brief  配置看门狗的两个引脚
  * @retval None
  * @note   EN 配成推挽输出；WDI 也配成推挽输出，但速度设为 HIGH，
  *         因为它是高速翻转的方波信号
  */
void WDOG_Port_Init(void);

/**
  * @brief  使能外部看门狗（EN 脚拉低）
  * @retval None
  */
void WDOG_Enable(void);

/**
  * @brief  关闭外部看门狗（EN 脚拉高）
  * @retval None
  * @note   进入 STOP 睡眠前必须调用，否则 MCU 停住不再喂狗，会被复位
  */
void WDOG_Disnable(void);

/**
  * @brief  喂狗：把 WDI 脚电平翻转一次
  * @retval None
  */
void WDOG_Feed(void);


#ifdef __cplusplus
}
#endif
#endif

