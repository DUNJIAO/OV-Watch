#include <math.h>
#include "SPL06_001.h"

/* 开启 SPL06_001 所在端口 GPIOB 的时钟 */
#define SPL_CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE()

/**
  * @brief  SPL06_001 的总线定义：SDA = PB13，SCL = PB14
  * @note   与 AHT21 共用同一组引脚，靠器件地址 0x76 区分
  */
iic_bus_t SPL_bus =
{
	.IIC_SDA_PORT = GPIOB,
	.IIC_SCL_PORT = GPIOB,
	.IIC_SDA_PIN  = GPIO_PIN_13,
	.IIC_SCL_PIN  = GPIO_PIN_14,
};

/**
  * @brief  出厂标定系数，SPL_init 时读一次，之后所有计算都用它们
  * @note   c0~c30 中只有 c00、c10 是 20 位，其余是 12 位
  */
int16_t c0,c1,c01,c11,c20,c21,c30;
int32_t c00,c10;

/**
  * @brief  读一个寄存器
  * @param  addr 寄存器地址
  * @retval 读到的字节
  */
uint8_t SPL_ReadOneReg(uint8_t addr)
{
	uint8_t dat;
	dat = IIC_Read_One_Byte(&SPL_bus, SPL_CHIP_ADDRESS,addr);
	return dat;
}

/**
  * @brief  写一个寄存器
  * @param  addr 寄存器地址
  * @param  dat  要写入的字节
  * @retval None
  * @note   底层写函数的返回值被直接丢弃
  */
void SPL_WriteOneReg(uint8_t addr, uint8_t dat)
{
	IIC_Write_One_Byte(&SPL_bus, SPL_CHIP_ADDRESS,addr,dat);
}

/**
  * @brief  读温度原始值（24 位补码）
  * @retval 原始温度值，带符号
  * @note   三个字节是从低到高读的，所以先读 B0，拼的时候反过来
  */
int32_t Get_Traw()
{
	uint8_t buff[3];
	int32_t Traw;
	buff[0] = SPL_ReadOneReg(SPL_TMP_B0);
	buff[1] = SPL_ReadOneReg(SPL_TMP_B1);
	buff[2] = SPL_ReadOneReg(SPL_TMP_B2);
	Traw = buff[2];
	Traw = Traw << 8 | buff[1];
	Traw = Traw << 8 | buff[0];
	if(Traw & (1<<23))
	{Traw |= 0xFF000000;}//24 bit 2´s complement numbers
	return Traw;
}

/**
  * @brief  读气压原始值（24 位补码）
  * @retval 原始气压值，带符号
  */
int32_t Get_Praw()
{
	uint8_t buff[3];
	int32_t Praw;
	buff[0] = SPL_ReadOneReg(SPL_PRS_B0);
	buff[1] = SPL_ReadOneReg(SPL_PRS_B1);
	buff[2] = SPL_ReadOneReg(SPL_PRS_B2);
	Praw = buff[2];
	Praw = Praw << 8 | buff[1];
	Praw = Praw << 8 | buff[0];
	if(Praw & (1<<23))
	{Praw |= 0xFF000000;}//24 bit 2´s complement numbers
	return Praw;
}

/**
  * @brief  读补偿系数 c0（12 位补码）
  * @retval c0
  * @note   0x10 是 c0 的高 8 位，0x11 的高 4 位是 c0 的低 4 位
  */
int16_t get_c0()
{
	uint8_t buff[2];
	int16_t c0;
	buff[0] = SPL_ReadOneReg(COEF_C0);
	buff[1] = SPL_ReadOneReg(COEF_C0_C1);
	c0 = buff[0];
	c0 = (c0 << 4) | (buff[1] >> 4);
	if(c0 & (1<<11))
	{c0 |= 0xF000;}
	return c0;
}

/**
  * @brief  读补偿系数 c1（12 位补码）
  * @retval c1
  * @note   c1 的低 4 位在 0x11 的低 4 位，高 8 位在 0x12
  */
int16_t get_c1()
{
	uint8_t buff[2];
	int16_t c1;
	buff[0] = SPL_ReadOneReg(COEF_C0_C1);
	buff[1] = SPL_ReadOneReg(COEF_C1);
	c1 = buff[0] & 0x0F;
	c1 = (c1 << 8) | buff[1] ;
	if(c1 & (1<<11))
	{c1 |= 0xF000;}
	return c1;
}

/**
  * @brief  读补偿系数 c00（20 位补码）
  * @retval c00
  */
int32_t get_c00()
{
	uint8_t buff[3];
	int32_t c00;
	buff[0] = SPL_ReadOneReg(COEF_C00_H);
	buff[1] = SPL_ReadOneReg(COEF_C00_L);
	buff[2] = SPL_ReadOneReg(COEF_C00_C10);
	c00 = buff[0];
	c00 = c00<<8 | buff[1];
	c00 = (c00<<4) | (buff[2]>>4);
	if(c00 & (1<<19))
	{c00 |= 0xFFF00000;}
	return c00;
}

/**
  * @brief  读补偿系数 c10（20 位补码）
  * @retval c10
  */
int32_t get_c10()
{
	uint8_t buff[3];
	int32_t c10;
	buff[0] = SPL_ReadOneReg(COEF_C00_C10);
	buff[1] = SPL_ReadOneReg(COEF_C10_M);
	buff[2] = SPL_ReadOneReg(COEF_C10_L);
	c10 = buff[0] & 0x0F;
	c10 = c10<<8 | buff[1];
	c10 = c10<<8 | buff[2];
	if(c10 & (1<<19))
	{c10 |= 0xFFF00000;}
	return c10;
}

/**
  * @brief  读补偿系数 c01（16 位）
  * @retval c01
  */
int16_t get_c01()
{
	uint8_t buff[2];
	int32_t c01;
	buff[0] = SPL_ReadOneReg(COEF_C01_H);
	buff[1] = SPL_ReadOneReg(COEF_C01_L);
	c01 = buff[0];
	c01 = c01<<8 | buff[1];
	return c01;
}

/**
  * @brief  读补偿系数 c11（16 位）
  * @retval c11
  */
int16_t get_c11()
{
	uint8_t buff[2];
	int32_t c11;
	buff[0] = SPL_ReadOneReg(COEF_C11_H);
	buff[1] = SPL_ReadOneReg(COEF_C11_L);
	c11 = buff[0];
	c11 = c11<<8 | buff[1];
	return c11;
}

/**
  * @brief  读补偿系数 c20（16 位）
  * @retval c20
  */
int16_t get_c20()
{
	uint8_t buff[2];
	int32_t c20;
	buff[0] = SPL_ReadOneReg(COEF_C20_H);
	buff[1] = SPL_ReadOneReg(COEF_C20_L);
	c20 = buff[0];
	c20 = c20<<8 | buff[1];
	return c20;
}

/**
  * @brief  读补偿系数 c21（16 位）
  * @retval c21
  */
int16_t get_c21()
{
	uint8_t buff[2];
	int32_t c21;
	buff[0] = SPL_ReadOneReg(COEF_C21_H);
	buff[1] = SPL_ReadOneReg(COEF_C21_L);
	c21 = buff[0];
	c21 = c21<<8 | buff[1];
	return c21;
}

/**
  * @brief  读补偿系数 c30（16 位）
  * @retval c30
  */
int16_t get_c30()
{
	uint8_t buff[2];
	int32_t c30;
	buff[0] = SPL_ReadOneReg(COEF_C30_H);
	buff[1] = SPL_ReadOneReg(COEF_C30_L);
	c30 = buff[0];
	c30 = c30<<8 | buff[1];
	return c30;
}

/**
  * @brief  初始化：开时钟、配置测量参数、读出 9 个补偿系数并回读校验
  * @retval 0：成功；1：回读校验失败
  * @note   配置项：气压 2 倍过采样、温度用外部源、连续测量模式、结果不移位
  */
uint8_t SPL_init()
{

	SPL_CLK_ENABLE;
	IICInit(&SPL_bus);

	SPL_WriteOneReg(SPL_PRS_CFG, 0x01);		// Pressure 2x oversampling

	SPL_WriteOneReg(SPL_TMP_CFG, 0x80);		// External Temperature 1x oversampling

	SPL_WriteOneReg(SPL_MEAS_CFG, 0x07);	// continuous pressure and temperature measurement

	SPL_WriteOneReg(SPL_CFG_REG, 0x00);		//

	c0 = get_c0();
	c1 = get_c1();
	c01 = get_c01();
	c11 = get_c11();
	c20 = get_c20();
	c21 = get_c21();
	c30 = get_c30();
	c00 = get_c00();
	c10 = get_c10();

	if(SPL_ReadOneReg(SPL_PRS_CFG)!=0x01 || SPL_ReadOneReg(SPL_CFG_REG)!=0x00)
	{return 1;}//ERRO
	else
	{return 0;}//SUCCESS
}

/**
  * @brief  进入待机，停止测量
  * @retval None
  */
void SPL_Sleep()
{
	SPL_WriteOneReg(SPL_MEAS_CFG, 0x00);	// standby
	//SPL_WriteOneReg(SPL_RESET_REG,0x09);//reset
}

/**
  * @brief  退出待机，恢复连续温压测量
  * @retval None
  */
void SPL_Wakeup()
{
	SPL_WriteOneReg(SPL_MEAS_CFG, 0x07);	// continuous pressure and temperature measurement
	//SPL_init();
}

/**
  * @brief  读芯片 ID
  * @retval ID 值，正常应为 0x10
  */
uint8_t SPL_GetID()
{
	return SPL_ReadOneReg(SPL_ID_REG);
}

/**
  * @brief  用已读出的系数计算温度
  * @retval 温度，单位 摄氏度
  * @note   KT 是把 24 位原始值归一化到 ±1 的缩放常数
  */
float Temperature_Calculate()
{
	float Traw_sc, Tcomp;
	Traw_sc = Get_Traw();
	Traw_sc /= KT;
	Tcomp = c0 * 0.5 + c1 * Traw_sc;
	return Tcomp;
}

/**
  * @brief  用已读出的系数计算气压
  * @retval 气压，单位 Pa
  * @note   多项式按数据手册给的 c00/c10/c20/c30/c01/c11/c21 展开
  */
float Pressure_Calculate()
{
	float Traw_sc, Praw_sc, Pcomp;
	Traw_sc = Get_Traw();
	Traw_sc /= KT;
	Praw_sc = Get_Praw();
	Praw_sc /= KP;
	Pcomp = (c00) + Praw_sc * ((c10) + Praw_sc * ((c20) + Praw_sc * (c30))) + Traw_sc * (c01) + Traw_sc * Praw_sc * ((c11) + Praw_sc * (c21));

	return Pcomp;
}

/**
  * @brief  由气压换算海拔
  * @retval 海拔，单位 米
  * @note   按海平面气压 101325 Pa 的标准大气公式
  */
float Altitude_Calculate()
{
	float Altitude;
	Altitude = 44330 * (1 - powf(Pressure_Calculate()/101325, 0.1903));
	return Altitude;
}
