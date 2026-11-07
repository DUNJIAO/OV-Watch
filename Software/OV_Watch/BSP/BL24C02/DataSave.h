#ifndef __DATASAVE_H__
#define __DATASAVE_H__

#include "BL24C02.h"

/**
  * @brief  初始化 EEPROM（实际就是 BL24C02_Init）
  * @retval None
  */
void EEPROM_Init(void);

/**
  * @brief  检查 EEPROM 是否可用
  * @retval 0：正常；1：读写异常
  * @note   读 0x00、0x01 两个校验字，对不上就写入 0x55、0xAA 再读回来验证
  */
uint8_t EEPROM_Check(void);

/**
  * @brief  把数据写入 EEPROM 的指定地址
  * @param  buf   数据缓冲区
  * @param  addr  起始地址，必须大于 1（0、1 是校验字）
  * @param  lenth 字节数
  * @retval 0：成功；1：失败或地址非法
  */
uint8_t SettingSave(uint8_t *buf, uint8_t addr, uint8_t lenth);

/**
  * @brief  从 EEPROM 的指定地址读数据
  * @param  buf   接收缓冲区
  * @param  addr  起始地址，必须大于 1（0、1 是校验字）
  * @param  lenth 字节数
  * @retval 0：成功；1：失败或地址非法
  */
uint8_t SettingGet(uint8_t *buf, uint8_t addr, uint8_t lenth);

#endif
