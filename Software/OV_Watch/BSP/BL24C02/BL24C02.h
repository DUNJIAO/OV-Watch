#ifndef __BL24C02_H
#define __BL24C02_H

#include "stm32f4xx_hal.h"
#include "iic_hal.h"
#include "delay.h"

/*
 * BL24C02：2Kbit EEPROM，共 256 字节
 * I2C 接口，7 位地址 0x50，SDA = PA11，SCL = PA12
 * 没有寄存器概念，读写的 addr 就是第几个字节
 */

#define BL_ADDRESS	0x50

/**
  * @brief  从指定地址开始连续写多个字节
  * @param  addr   起始字节地址
  * @param  length 要写的字节数
  * @param  buff   数据缓冲区，由调用方准备
  * @retval None
  * @note   写完器件内部还需要约 5 ms 的写周期，这期间它不响应；
  *         连续写要在两次之间留出延时
  */
void BL24C02_Write(uint8_t addr,uint8_t length,uint8_t buff[]);

/**
  * @brief  从指定地址开始连续读多个字节
  * @param  addr   起始字节地址
  * @param  length 要读的字节数
  * @param  buff   接收缓冲区，由调用方准备
  * @retval None
  */
void BL24C02_Read(uint8_t addr, uint8_t length, uint8_t buff[]);

/**
  * @brief  初始化：开 GPIOA 时钟并初始化软件 I2C
  * @retval None
  */
void BL24C02_Init(void);

#endif
