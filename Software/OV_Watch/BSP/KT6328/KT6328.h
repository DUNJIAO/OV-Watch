#ifndef __KT6328_H__
#define __KT6328_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/*
 * KT6328：蓝牙模块的电源/使能控制
 * 只占用一根 GPIO：PA8
 * 高电平 = 打开模块，低电平 = 关闭
 * 真正的 BLE 协议栈不在这里，这里只负责上电
 */

/**
  * @brief  配置 PA8 为推挽输出，初始输出低电平
  * @retval None
  * @note   先写低电平再配置模式，避免配置瞬间输出高电平误开模块
  */
void KT6328_GPIO_Init(void);

/**
  * @brief  打开蓝牙模块（PA8 输出高电平）
  * @retval None
  */
void KT6328_Enable(void);

/**
  * @brief  关闭蓝牙模块（PA8 输出低电平）
  * @retval None
  */
void KT6328_Disable(void);


#ifdef __cplusplus
}
#endif
#endif

