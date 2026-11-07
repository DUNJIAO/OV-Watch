#include "DataSave.h"
#include "string.h"

/******************************************
EEPROM Data description:
[0x00]:0x55 for check
[0x01]:0xAA for check

[0x10]:user wrist setting, HWInterface.IMU.wrist_is_enabled
[0x11]:user ui_APPSy_EN setting

[0x20]:Last Save Day(0-31)
[0x21]:Day Steps

*******************************************/


/**
  * @brief  初始化 EEPROM
  * @retval None
  * @note   只是 BL24C02_Init 的别名，这里没有做任何数据检查
  */
void EEPROM_Init(void)
{
	BL24C02_Init();
}


/**
  * @brief  检查 EEPROM 是否可用
  * @retval 0：正常；1：读写异常
  * @note   先读 0x00、0x01；不是 0x55、0xAA 就写入这对校验字，再读回来确认。
  *         相当于"新芯片首次使用"的初始化流程。
  *         三次 delay_ms(10) 是给 EEPROM 内部写周期留时间
  */
//to check the Data is right and the EEPROM is working
uint8_t EEPROM_Check(void)
{
	uint8_t check_buff[2];
	delay_ms(10);
	BL24C02_Read(0,2,check_buff);
	if(check_buff[0] == 0x55 && check_buff[1] == 0xAA)
	{
		return 0;//check OK
	}
	else
	{
		check_buff[0] = 0x55;
		check_buff[1] = 0xAA;
		delay_ms(10);
		BL24C02_Write(0,2,check_buff);
		memset(check_buff,0,2);
		delay_ms(10);
		BL24C02_Read(0,2,check_buff);
		if(check_buff[0] == 0x55 && check_buff[1] == 0xAA)
			return 0;//check ok
	}
	return 1;//check erro
}


/**
  * @brief  把数据写入 EEPROM 的指定地址
  * @param  buf   数据缓冲区
  * @param  addr  起始地址，必须大于 1
  * @param  lenth 字节数
  * @retval 0：成功；1：失败或地址非法
  * @note   地址 0、1 是校验字，被 addr > 1 挡住不许写；
  *         每次调用都会先跑一遍 EEPROM_Check，固定多出 10 ms 以上开销
  */
//to Save the settings
uint8_t SettingSave(uint8_t *buf, uint8_t addr, uint8_t lenth)
{
	if(addr > 1 && !EEPROM_Check())
	{
		delay_ms(10);
		BL24C02_Write(addr,lenth,buf);
		return 0;
	}
	return 1;
}


/**
  * @brief  从 EEPROM 的指定地址读数据
  * @param  buf   接收缓冲区
  * @param  addr  起始地址，必须大于 1
  * @param  lenth 字节数
  * @retval 0：成功；1：失败或地址非法
  * @note   同样先跑一遍 EEPROM_Check
  */
//to Get the settings
uint8_t SettingGet(uint8_t *buf, uint8_t addr, uint8_t lenth)
{
	if(addr > 1 && !EEPROM_Check())
	{
		delay_ms(10);
		BL24C02_Read(addr,lenth,buf);
		return 0;
	}
	return 1;
}
