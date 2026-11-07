#ifndef __EM70X8_H__
#define __EM70X8_H__

#include "iic_hal.h"

/*
 * EM7028 心率传感器，I2C 接口，7 位地址 0x24，ID 寄存器应读到 0x36
 * SDA = PB13，SCL = PB14，与 AHT21 / SPL06 / LSM303 共用
 * 原始值由 HrAlgorythm.c 的 HR_Calculate 做峰值检测，换算成 bpm
 */

#define EM7028_ID					0x36
#define EM7028_ADDR				0x24

#define ID_REG						0x00
#define HRS_CFG						0x01
#define HRS_INT_CTRL			0x02
#define HRS_LT_L					0X03
#define HRS_LT_H					0x04
#define HRS_HT_L					0x05
#define HRS_HT_H					0x06
#define LED_CRT						0x07
#define	HRS2_DATA_OFFSET	0x08
#define HRS2_CTRL					0x09
#define HRS2_GAIN_CTRL		0x0A
#define HRS1_CTRL					0x0D
#define INT_CTRL					0x0E
#define SOFT_RESET				0x0F

#define HRS2_DATA0_L		0x20
#define HRS2_DATA0_H		0x21
#define HRS2_DATA1_L		0x22
#define HRS2_DATA1_H		0x23
#define HRS2_DATA2_L		0x24
#define HRS2_DATA2_H		0x25
#define HRS2_DATA3_L		0x26
#define HRS2_DATA3_H		0x27

#define HRS1_DATA0_L		0x28
#define HRS1_DATA0_H		0x29
#define HRS1_DATA1_L		0x2A
#define HRS1_DATA1_H		0x2B
#define HRS1_DATA2_L		0x2C
#define HRS1_DATA2_H		0x2D
#define HRS1_DATA3_L		0x2E
#define HRS1_DATA3_H		0x2F

//读一个寄存器
/**
  * @brief  读一个寄存器
  * @param  RegAddr 寄存器地址
  * @retval 读到的字节
  */
uint8_t  EM7028_ReadOneReg(unsigned char RegAddr);

/**
  * @brief  写一个寄存器
  * @param  RegAddr 寄存器地址
  * @param  dat     要写入的字节
  * @retval None
  */
void  EM7028_WriteOneReg(unsigned char RegAddr, unsigned char dat);

/**
  * @brief  读芯片 ID
  * @retval ID 值，正常应为 0x36
  */
uint8_t EM7028_Get_ID(void);

/**
  * @brief  初始化：开时钟、初始化总线、等器件就绪、写入 5 个配置寄存器
  * @retval 0：成功；1：等不到 ID = 0x36（器件没接上）
  * @note   等 ID 最多 5 次 × 100 ms
  */
uint8_t EM7028_hrs_init(void);

/**
  * @brief  打开心率测量
  * @retval 0：成功；1：器件没响应
  * @note   写 HRS_CFG = 0x08，即 bit3（HRS_EN）置 1
  */
uint8_t EM7028_hrs_Enable(void);

/**
  * @brief  关闭心率测量
  * @retval 0：成功；1：器件没响应
  * @note   写 HRS_CFG = 0x00
  */
uint8_t EM7028_hrs_DisEnable(void);

/*
* @brife :获取HRS1传感器原始值
* 返回值: HRS1传感器原始值
*/
/**
  * @brief  读取 HRS1 传感器的原始值
  * @retval 16 位原始值（高字节在前拼进来）
  */
uint16_t EM7028_Get_HRS1(void);

#endif
