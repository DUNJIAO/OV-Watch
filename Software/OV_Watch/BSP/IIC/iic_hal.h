#ifndef __IIC_HAL_H
#define __IIC_HAL_H

#include "stm32f4xx_hal.h"

//所有 IIC 函数的第一个参数都是 iic_bus_t *bus，
//这样同一套代码可以服务多个设备：
//每个传感器在初始化时填入自己的端口和引脚，后面所有函数都通过这个指针读取。
typedef struct
{
	GPIO_TypeDef * IIC_SDA_PORT;
	GPIO_TypeDef * IIC_SCL_PORT;
	uint16_t IIC_SDA_PIN;
	uint16_t IIC_SCL_PIN;
	//void (*CLK_ENABLE)(void);
}iic_bus_t;

void IICStart(iic_bus_t *bus);
void IICStop(iic_bus_t *bus);
unsigned char IICWaitAck(iic_bus_t *bus);
void IICSendAck(iic_bus_t *bus);
void IICSendNotAck(iic_bus_t *bus);
void IICSendByte(iic_bus_t *bus, unsigned char cSendByte);
unsigned char IICReceiveByte(iic_bus_t *bus);
void IICInit(iic_bus_t *bus);

uint8_t IIC_Write_One_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg,uint8_t data);
uint8_t IIC_Write_Multi_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg,uint8_t length,uint8_t buff[]);
unsigned char IIC_Read_One_Byte(iic_bus_t *bus, uint8_t daddr,uint8_t reg);
uint8_t IIC_Read_Multi_Byte(iic_bus_t *bus, uint8_t daddr, uint8_t reg, uint8_t length, uint8_t buff[]);
#endif
