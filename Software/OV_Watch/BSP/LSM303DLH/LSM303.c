#include "LSM303.h"
#include <math.h>

#define PI 3.1415926
#define Delayms(X) HAL_Delay(X)

/* 开启 LSM303DLH 所在端口 GPIOB 的时钟 */
#define LSM303_CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE()

/**
  * @brief  LSM303DLH 的总线定义：SDA = PB13，SCL = PB14
  * @note   一个模块两个从机地址：加速度 0x19，磁力计 0x1E
  */
iic_bus_t LSM303_bus =
{
	.IIC_SDA_PORT = GPIOB,
	.IIC_SCL_PORT = GPIOB,
	.IIC_SDA_PIN  = GPIO_PIN_13,
	.IIC_SCL_PIN  = GPIO_PIN_14,
};

/* NOTICE: the LSM303 can not use mutiple read! */

/**
  * @brief  读一个寄存器
  * @param  RegAddr 寄存器地址
  * @retval 读到的字节
  * @note   按地址自动选从机：> 0x19 走加速度（0x19），否则走磁力计（0x1E）
  */
unsigned char LSM303_ReadOneReg(unsigned char RegAddr)
{
	unsigned char dat;
	unsigned char SlaveAddr = (RegAddr>0x19)?LSM303_SlaveAddr_A:LSM303_SlaveAddr_M;
	dat = IIC_Read_One_Byte(&LSM303_bus, SlaveAddr,RegAddr);
	return dat;
}

/**
  * @brief  连续读多个相邻寄存器
  * @param  RegAddr  起始寄存器地址
  * @param  RegNum   要读的字节数
  * @param  DataBuff 接收缓冲区，由调用方准备
  * @retval None
  * @note   这颗器件不支持连续读，所以这里是循环单字节读，
  *         每个字节都要走一遍完整的起始/地址/寄存器/读/停止
  */
void LSM303_ReadMultiReg(unsigned char RegAddr, unsigned char RegNum, unsigned char DataBuff[])
{
	unsigned char i;
	unsigned char SlaveAddr = (RegAddr>0x19)?LSM303_SlaveAddr_A:LSM303_SlaveAddr_M;
	//IIC_Read_Multi_Byte(SlaveAddr,RegAddr,RegNum,DataBuff);
	for(i=0;i<RegNum;i++)
	{
		DataBuff[i] = IIC_Read_One_Byte(&LSM303_bus, SlaveAddr,RegAddr+i);
	}
}

/**
  * @brief  读温度寄存器
  * @param  RegAddr 寄存器地址
  * @retval 读到的字节
  * @note   固定走磁力计地址 0x1E
  */
unsigned char LSM303_Temp_ReadOneReg(unsigned char RegAddr)
{
	unsigned char dat;
	dat = IIC_Read_One_Byte(&LSM303_bus, LSM303_SlaveAddr_M,RegAddr);
	return dat;
}

/**
  * @brief  写一个寄存器
  * @param  RegAddr 寄存器地址
  * @param  dat     要写入的字节
  * @retval None
  * @note   按地址自动选从机，规则同 ReadOneReg
  */
void LSM303_WriteOneReg(unsigned char RegAddr, unsigned char dat)
{
	unsigned char SlaveAddr = (RegAddr>0x19)?LSM303_SlaveAddr_A:LSM303_SlaveAddr_M;
	IIC_Write_One_Byte(&LSM303_bus, SlaveAddr,RegAddr,dat);
}

/**
  * @brief  初始化加速度计和磁力计，写完配置后回读校验
  * @retval 0：成功；1：加速度或磁力计配置没写进去
  * @note   加速度：CTRL_REG4 = 0x10（±4g），CTRL_REG1 = 0x2F（低功耗 10Hz）
  *         磁力计：CRA = 0x10（15Hz），CRB = 0x80（±4.0 Gauss），MR = 0x00（连续转换）
  *         各最多重试 3 次，回读一致才算成功
  */
unsigned char LSM303DLH_Init()
{
	unsigned char temp;
	unsigned char retry = 0;

	LSM303_CLK_ENABLE;
	IICInit(&LSM303_bus);

	for(retry = 0;retry < 3;retry ++)
	{
		LSM303_WriteOneReg(LSM303_CTRL_REG4_A, 0x10);		//full scale: +-4g
		Delayms(1);
		LSM303_WriteOneReg(LSM303_CTRL_REG1_A, 0x2F);		//low power mode, 10Hz速度
		Delayms(1);
		temp = LSM303_ReadOneReg(LSM303_CTRL_REG1_A);		//读取0x20寄存器，默认值为0x07
		if(temp != 0x2F)
		{
			Delayms(10);
		}
		else break;
	}
	if(temp != 0x2F)
	{
		return 1;
	}
	Delayms(1);
	for(retry = 0;retry < 3;retry ++)
	{
		LSM303_WriteOneReg(LSM303_CRA_REG_M, 0x10);		//磁场传感器15Hz,正常测量;关闭温度计
		Delayms(1);
		LSM303_WriteOneReg(LSM303_CRB_REG_M, 0x80);		//磁场传感器gain默认为0x80: +-4.0Gauss, 450LBS/Gauss for XY, 400LBS/Gauss for Z
		Delayms(1);
		LSM303_WriteOneReg(LSM303_MR_REG_M, 0x00);		//磁场传感器连续转换模式
		Delayms(1);
		temp = LSM303_ReadOneReg(LSM303_MR_REG_M);
		if(temp != 0)
		{
			Delayms(10);
		}
		else break;
	}
	if(temp != 0)
	{
		return 1;
	}
	return 0;
}

/**
  * @brief  两个器件都进入低功耗
  * @retval None
  * @note   磁力计设为 0x03（待机），加速度 CTRL_REG1 设为 0x0F（关掉大部分使能位）
  */
void LSM303DLH_Sleep()
{
	LSM303_WriteOneReg(LSM303_MR_REG_M,0x03);
	LSM303_WriteOneReg(LSM303_CTRL_REG1_A,0x0f);
}

/**
  * @brief  恢复连续测量
  * @retval None
  */
void LSM303DLH_Wakeup()
{
	LSM303_WriteOneReg(LSM303_MR_REG_M,0x00);//磁场传感器连续转换模式
	LSM303_WriteOneReg(LSM303_CTRL_REG1_A,0x2f);//low power mode, 10Hz速度
}

/**
  * @brief  读三轴加速度
  * @param  Xa,Ya,Za 输出参数，原始 LSB 值
  * @retval None
  * @note   加速度寄存器低字节在前，所以先拼 buff[1] 再拼 buff[0]
  *         要换算成 g，还需按量程除以对应灵敏度
  */
void LSM303_ReadAcceleration(int16_t *Xa, int16_t *Ya, int16_t *Za)
{
	uint8_t buff[6];
	int16_t temp;
	LSM303_ReadMultiReg(LSM303_OUT_X_L_A,6,buff);

	temp = buff[1];
	temp <<= 8;
	temp |= buff[0];
	*Xa = temp;

	temp = buff[3];
	temp <<= 8;
	temp |= buff[2];
	*Ya = temp;

	temp = buff[5];
	temp <<= 8;
	temp |= buff[4];
	*Za = temp;
}

/**
  * @brief  读三轴磁场
  * @param  Xm,Ym,Zm 输出参数，原始 LSB 值
  * @retval None
  * @note   磁场寄存器是"高字节在前"，而且排列顺序是 X、Z、Y
  *         所以 buff[2..3] 拼出来给的是 Zm，buff[4..5] 才是 Ym
  */
void LSM303_ReadMagnetic(int16_t *Xm, int16_t *Ym, int16_t *Zm)
{
	uint8_t buff[6];
	int16_t temp;
	LSM303_ReadMultiReg(LSM303_OUT_X_H_M,6, buff);

	temp = buff[0];
	temp <<= 8;
	temp |= buff[1];
	*Xm = temp;

	temp = buff[2];
	temp <<= 8;
	temp |= buff[3];
	*Zm = temp;

	temp = buff[4];
	temp <<= 8;
	temp |= buff[5];
	*Ym = temp;
}

/**
  * @brief  读温度
  * @param  Temp 输出参数，摄氏度
  * @retval None
  * @note   12 位数据在高 12 位，先右移 4 位；
  *         数据手册不保证换算系数，这里按 8 LSB/度 除 3 处理
  */
void LSM303_ReadTemperature(int16_t *Temp)
{
	uint8_t buff[2];
	int16_t temp;
	IIC_Read_Multi_Byte(&LSM303_bus, LSM303_SlaveAddr_M, TEMP_OUT_H_M, 2, buff);
	temp = buff[0];
	temp <<= 8;
	temp |= buff[1];
	temp >>= 4;
	*Temp = temp / 3 + 0.5;//8 LSB/deg
}

/**
  * @brief  计算 Z 轴倾角
  * @param  Xa,Ya,Za 三轴加速度原始值
  * @retval 角度 × 100，单位 0.01 度
  * @note   先用三轴合成矢量归一化，再对 Z 求夹角
  */
int LSM303DLH_CalculationZAxisAngle(int16_t Xa, int16_t Ya, int16_t Za)
{
	double A;
	float fx,fy,fz;

	A = sqrt((int)Xa*Xa + (int)Ya*Ya + (int)Za*Za);	//计算角加速度的矢量模长 |A|=根号下(X*X+Y*Y+Z*Z)
	fx = Xa/A;
	fy = Ya/A;
	fz = Za/A;


	//Z方向
	A = fx*fx+fy*fy;
	A = sqrt(A);
	A = (double)A/fz;
	A = atan(A);
	A = A*180/PI;
	if(A < 0)
	{
		A += 90;
		A = 0-A;
	}
	else
	{
		A = 90-A;
	}
	return A*100;
}


/**
  * @brief  计算 X 轴倾角
  * @param  Xa,Ya,Za 三轴加速度原始值
  * @retval 角度 × 100，单位 0.01 度
  * @note   向上为正，向下为负
  */
int LSM303DLH_CalculationXAxisAngle(int16_t Xa, int16_t Ya, int16_t Za)
{
	double A;
	float fx,fy,fz;

	A = sqrt((int)Xa*Xa + (int)Ya*Ya + (int)Za*Za);	//计算角加速度的矢量模长 |A|=根号下(X*X+Y*Y+Z*Z)
	fx = Xa/A;
	fy = Ya/A;
	fz = Za/A;


	//X方向
	A = fz*fz+fy*fy;
	A = sqrt(A);

	A = (double)A/fx;
	A = atan(A);
	A = A*180/PI;
	if(A < 0)
	{
		A += 90; //向上为正
	}
	else
	{
		A = 90-A;
		A = 0-A; //向下为负
	}
	return A*100;
}

/**
  * @brief  由加速度和磁场算方位角
  * @param  Xa,Ya,Za 三轴加速度；Xm,Ym,Zm 三轴磁场
  * @retval 方位角，单位 度
  * @note   先用加速度算出 pitch、roll 做倾斜补偿，
  *         再把磁场投影到水平面，最后 atan2 得到方向角
  */
float Azimuth_Calculate(int16_t Xa, int16_t Ya, int16_t Za, int16_t Xm, int16_t Ym, int16_t Zm)
{
	float pitch, roll, Hy, Hx, Azimuth;
	pitch   = atan2f(Xa, sqrtf(Ya * Ya + Za * Za));
	roll    = atan2f(Ya, sqrtf(Xa * Xa + Za * Za));
	Hy      = Ym * cosf(roll) + Xm * sinf(roll) * sinf(pitch) - Zm * cosf(pitch) * sinf(roll);
	Hx      = Xm * cosf(pitch) + Zm * sinf(pitch);
	Azimuth = atan2f(Hy,Hx)*180.0/PI;
	return Azimuth;
}
