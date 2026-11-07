#ifndef __AHT21_H
#define __AHT21_H

#include "stm32f4xx_hal.h"
#include "iic_hal.h"
#include "delay.h"

/*
 * AHT21 温湿度传感器，I2C 接口，7 位地址 0x38（线上为 0x70 写 / 0x71 读）
 * SDA = PB13，SCL = PB14，见 AHT21.c 中的 AHT_bus
 */

/**
  * @brief  读取器件状态字节
  * @retval 状态字节：bit7 = 1 表示正在测量，bit3 = 1 表示已标定
  */
uint8_t AHT_Read_Status(void);

/**
  * @brief  软复位，向器件发送命令 0xBA
  * @note   复位后需等待上电稳定时间才能继续读数
  * @retval None
  */
void AHT_Reset(void);

/**
  * @brief  初始化：开启 GPIOB 时钟并初始化软件 I2C
  * @note   本意为未标定时发送 0xBE 0x08 0x00 初始化命令，
  *         但 AHT21.c:52 的条件误写成 &&，该段实际不会执行
  * @retval 固定为 0，不代表成败
  */
uint8_t AHT_Init(void);

/**
  * @brief  触发一次测量并读取结果
  * @param  humi  输出参数，湿度，单位 %RH
  * @param  temp  输出参数，温度，单位 摄氏度
  * @note   两个 float 变量由调用方定义，本函数只接收地址
  * @retval 0：成功；1：等待测量完成超时
  */
uint8_t AHT_Read(float *humi, float *temp);

#endif
