#include "BL24C02.h"

/* 开启 BL24C02 所在端口 GPIOA 的时钟 */
#define BL_CLK_ENABLE __HAL_RCC_GPIOA_CLK_ENABLE()

/**
  * @brief  BL24C02 的总线定义：SDA = PA11，SCL = PA12
  * @note   这是工程里第三组 I2C 引脚（另外两组是 PB13/14 和 PB4/6）
  */
iic_bus_t BL_bus =
{
	.IIC_SDA_PORT = GPIOA,
	.IIC_SCL_PORT = GPIOA,
	.IIC_SDA_PIN  = GPIO_PIN_11,
	.IIC_SCL_PIN  = GPIO_PIN_12,
};


/**
  * @brief  从指定地址开始连续写多个字节
  * @param  addr   起始字节地址
  * @param  length 要写的字节数
  * @param  buff   数据缓冲区
  * @retval None
  * @note   底层 IIC_Write_Multi_Byte 的返回值被丢弃，写失败无法感知；
  *         写完器件仍处于内部写周期（约 5 ms），这期间不会应答
  */
void BL24C02_Write(uint8_t addr,uint8_t length,uint8_t buff[])
{
	IIC_Write_Multi_Byte(&BL_bus, BL_ADDRESS, addr, length, buff);
}


/**
  * @brief  从指定地址开始连续读多个字节
  * @param  addr   起始字节地址
  * @param  length 要读的字节数
  * @param  buff   接收缓冲区
  * @retval None
  */
void BL24C02_Read(uint8_t addr, uint8_t length, uint8_t buff[])
{
	IIC_Read_Multi_Byte(&BL_bus, BL_ADDRESS, addr, length, buff);
}


/**
  * @brief  初始化：开 GPIOA 时钟并初始化软件 I2C
  * @retval None
  */
void BL24C02_Init(void)
{
	BL_CLK_ENABLE;
	IICInit(&BL_bus);
}
