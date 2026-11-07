#include "em70x8.h"

/* 开启 EM7028 所在端口 GPIOB 的时钟；注意宏里自带分号 */
#define CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE();

/**
  * @brief  EM7028 的总线定义：SDA = PB13，SCL = PB14
  * @note   与 AHT21 / SPL06 / LSM303 共用同一组引脚，靠地址 0x24 区分
  */
iic_bus_t EM7028_bus =
{
	.IIC_SDA_PORT = GPIOB,
	.IIC_SCL_PORT = GPIOB,
	.IIC_SDA_PIN  = GPIO_PIN_13,
	.IIC_SCL_PIN  = GPIO_PIN_14,
};

/**
  * @brief  读一个寄存器
  * @param  RegAddr 寄存器地址
  * @retval 读到的字节
  */
uint8_t  EM7028_ReadOneReg(unsigned char RegAddr)
{
	unsigned char dat;
	//通过IIC读取一个寄存器的值
	dat = IIC_Read_One_Byte(&EM7028_bus, EM7028_ADDR, RegAddr);
	return dat;
}

/**
  * @brief  写一个寄存器
  * @param  RegAddr 寄存器地址
  * @param  dat     要写入的字节
  * @retval None
  */
void  EM7028_WriteOneReg(unsigned char RegAddr, unsigned char dat)
{
	IIC_Write_One_Byte(&EM7028_bus, EM7028_ADDR, RegAddr, dat);
}

/**
  * @brief  读芯片 ID
  * @retval ID 值，正常应为 0x36
  */
uint8_t EM7028_Get_ID()
{
	return EM7028_ReadOneReg(ID_REG);
}

/**
  * @brief  初始化：开时钟、初始化总线、等器件就绪、写入 5 个配置寄存器
  * @retval 0：成功；1：等不到 ID = 0x36
  * @note   配置项依次为：HRS 测量关闭、偏移 0、HRS2 增益 1、
  *         HRS1 增益 1 / 量程 8 / 16 位、LED 电流 2.5 mA
  */
uint8_t EM7028_hrs_init()
{
	uint8_t i = 5;

	CLK_ENABLE;
	IICInit(&EM7028_bus);

	while(EM7028_Get_ID() != 0x36 && i)
	{
		HAL_Delay(100);
		i--;
	}
	if(!i)
	{return 1;}
	EM7028_WriteOneReg(HRS_CFG,0x00);
	//HRS1_EN, HRS2_dis
	//Heart Beat Measurement is enabled with LED1 turned on and only Red Light Sensor and IR sensor enabled.
	//When LED1 turned on, the result stores to HRS_DATA0;
	EM7028_WriteOneReg(HRS2_DATA_OFFSET, 0x00);
	//0 offset
	EM7028_WriteOneReg(HRS2_GAIN_CTRL, 0x7f);
	//HRS2 GAIN = 1
	EM7028_WriteOneReg(HRS1_CTRL, 0x47);
	//HRS1 GAIN =1, HRS1 RANGE =8, HRS1 FREQ = 2.62144MHz (1.5625ms), HRS1 RES = 16 bits, HRS1 mode
	EM7028_WriteOneReg(INT_CTRL, 0x00);
	//LED programmed current = 2.5mA
	return 0;
}

/**
  * @brief  打开心率测量
  * @retval 0：成功；1：器件没响应
  * @note   写 HRS_CFG = 0x08（bit3 HRS_EN 置 1）
  */
uint8_t EM7028_hrs_Enable()
{
	uint8_t i = 5;
	while(EM7028_Get_ID() != 0x36 && i)
	{
		HAL_Delay(100);
		i--;
	}
	if(!i)
	{return 1;}
	EM7028_WriteOneReg(HRS_CFG,0x08);
	return 0;
}

/**
  * @brief  关闭心率测量
  * @retval 0：成功；1：器件没响应
  * @note   写 HRS_CFG = 0x00
  */
uint8_t EM7028_hrs_DisEnable()
{
	uint8_t i = 5;
	while(EM7028_Get_ID() != 0x36 && i)
	{
		HAL_Delay(100);
		i--;
	}
	if(!i)
	{return 1;}
	EM7028_WriteOneReg(HRS_CFG,0x00);
	return 0;
}

/**
  * @brief  读取 HRS1 传感器的原始值
  * @retval 16 位原始值（先读高字节，左移 8 位再拼低字节）
  * @note   读的是 HRS1_DATA0，HRS1 共有 4 组数据未用到这里
  */
uint16_t EM7028_Get_HRS1(void)
{
	uint16_t dat;
	dat = EM7028_ReadOneReg(HRS1_DATA0_H);
	dat <<= 8;
	dat |= EM7028_ReadOneReg(HRS1_DATA0_L);
	return dat;
}
