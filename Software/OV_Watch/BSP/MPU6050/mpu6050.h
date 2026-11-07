#ifndef __MPU6050_H
#define __MPU6050_H

#include "iic_hal.h"   												  	  
#include "sys.h"
#include "math.h"
#include "delay.h"

//#define MPU_ACCEL_OFFS_REG		0X06	//accel_offs寄存器,可读取版本号,寄存器手册未提到
//#define MPU_PROD_ID_REG				0X0C	//prod id寄存器,在寄存器手册未提到
#define MPU_SELF_TESTX_REG		0X0D	//自检寄存器X
#define MPU_SELF_TESTY_REG		0X0E	//自检寄存器Y
#define MPU_SELF_TESTZ_REG		0X0F	//自检寄存器Z
#define MPU_SELF_TESTA_REG		0X10	//自检寄存器A
#define MPU_SAMPLE_RATE_REG		0X19	//采样频率分频器
#define MPU_CFG_REG						0X1A	//配置寄存器
#define MPU_GYRO_CFG_REG			0X1B	//陀螺仪配置寄存器
#define MPU_ACCEL_CFG_REG			0X1C	//加速度计配置寄存器
#define MPU_MOTION_DET_REG		0X1F	//运动检测阀值设置寄存器
#define MPU_MOTION_DUR_REG		0X20	//motion time value
#define MPU_FIFO_EN_REG				0X23	//FIFO使能寄存器
#define MPU_I2CMST_CTRL_REG		0X24	//IIC主机控制寄存器
#define MPU_I2CSLV0_ADDR_REG	0X25	//IIC从机0器件地址寄存器
#define MPU_I2CSLV0_REG				0X26	//IIC从机0数据地址寄存器
#define MPU_I2CSLV0_CTRL_REG	0X27	//IIC从机0控制寄存器
#define MPU_I2CSLV1_ADDR_REG	0X28	//IIC从机1器件地址寄存器
#define MPU_I2CSLV1_REG				0X29	//IIC从机1数据地址寄存器
#define MPU_I2CSLV1_CTRL_REG	0X2A	//IIC从机1控制寄存器
#define MPU_I2CSLV2_ADDR_REG	0X2B	//IIC从机2器件地址寄存器
#define MPU_I2CSLV2_REG				0X2C	//IIC从机2数据地址寄存器
#define MPU_I2CSLV2_CTRL_REG	0X2D	//IIC从机2控制寄存器
#define MPU_I2CSLV3_ADDR_REG	0X2E	//IIC从机3器件地址寄存器
#define MPU_I2CSLV3_REG				0X2F	//IIC从机3数据地址寄存器
#define MPU_I2CSLV3_CTRL_REG	0X30	//IIC从机3控制寄存器
#define MPU_I2CSLV4_ADDR_REG	0X31	//IIC从机4器件地址寄存器
#define MPU_I2CSLV4_REG				0X32	//IIC从机4数据地址寄存器
#define MPU_I2CSLV4_DO_REG		0X33	//IIC从机4写数据寄存器
#define MPU_I2CSLV4_CTRL_REG	0X34	//IIC从机4控制寄存器
#define MPU_I2CSLV4_DI_REG		0X35	//IIC从机4读数据寄存器

#define MPU_I2CMST_STA_REG		0X36	//IIC主机状态寄存器
#define MPU_INTBP_CFG_REG			0X37	//中断/旁路设置寄存器
#define MPU_INT_EN_REG				0X38	//中断使能寄存器
#define MPU_INT_STA_REG				0X3A	//中断状态寄存器

#define MPU_ACCEL_XOUTH_REG		0X3B	//加速度值,X轴高8位寄存器
#define MPU_ACCEL_XOUTL_REG		0X3C	//加速度值,X轴低8位寄存器
#define MPU_ACCEL_YOUTH_REG		0X3D	//加速度值,Y轴高8位寄存器
#define MPU_ACCEL_YOUTL_REG		0X3E	//加速度值,Y轴低8位寄存器
#define MPU_ACCEL_ZOUTH_REG		0X3F	//加速度值,Z轴高8位寄存器
#define MPU_ACCEL_ZOUTL_REG		0X40	//加速度值,Z轴低8位寄存器

#define MPU_TEMP_OUTH_REG			0X41	//温度值高八位寄存器
#define MPU_TEMP_OUTL_REG			0X42	//温度值低8位寄存器

#define MPU_GYRO_XOUTH_REG		0X43	//陀螺仪值,X轴高8位寄存器
#define MPU_GYRO_XOUTL_REG		0X44	//陀螺仪值,X轴低8位寄存器
#define MPU_GYRO_YOUTH_REG		0X45	//陀螺仪值,Y轴高8位寄存器
#define MPU_GYRO_YOUTL_REG		0X46	//陀螺仪值,Y轴低8位寄存器
#define MPU_GYRO_ZOUTH_REG		0X47	//陀螺仪值,Z轴高8位寄存器
#define MPU_GYRO_ZOUTL_REG		0X48	//陀螺仪值,Z轴低8位寄存器

#define MPU_MOT_DET_STA_REG 	0x61	//motion detect status

#define MPU_I2CSLV0_DO_REG		0X63	//IIC从机0数据寄存器
#define MPU_I2CSLV1_DO_REG		0X64	//IIC从机1数据寄存器
#define MPU_I2CSLV2_DO_REG		0X65	//IIC从机2数据寄存器
#define MPU_I2CSLV3_DO_REG		0X66	//IIC从机3数据寄存器

#define MPU_I2CMST_DELAY_REG	0X67	//IIC主机延时管理寄存器
#define MPU_SIGPATH_RST_REG		0X68	//信号通道复位寄存器
#define MPU_MDETECT_CTRL_REG	0X69	//运动检测控制寄存器
#define MPU_USER_CTRL_REG			0X6A	//用户控制寄存器
#define MPU_PWR_MGMT1_REG			0X6B	//电源管理寄存器1
#define MPU_PWR_MGMT2_REG			0X6C	//电源管理寄存器2 
#define MPU_FIFO_CNTH_REG			0X72	//FIFO计数寄存器高八位
#define MPU_FIFO_CNTL_REG			0X73	//FIFO计数寄存器低八位
#define MPU_FIFO_RW_REG				0X74	//FIFO读写寄存器
#define MPU_DEVICE_ID_REG			0X75	//器件ID寄存器
 
//如果AD0脚(9脚)接地,IIC地址为0X68(不包含最低位).
//如果接V3.3,则IIC地址为0X69(不包含最低位).
#define MPU_ADDR				0X68
#define MPU_ID					0x68

////因为开发板接GND,所以转为读写地址后,为0XD1和0XD0(如果接GND,则为0XD3和0XD2)  
//#define MPU_READ    0XD1
//#define MPU_WRITE   0XD0

/**
  * @brief  配置中断脚 PB12（下降沿触发）并使能 EXTI15_10 中断
  * @retval None
  */

void MPU_INT_Pin_Init(void);
/**
  * @brief  打开 MPU6050 的运动检测中断
  * @retval None
  */

void MPU_Motion_Init(void);
/**
  * @brief  初始化 MPU6050 所在的软件 I2C 总线（PB13/PB14）
  * @retval None
  */

void MPU_Bus_Init(void);
/**
  * @brief  初始化 MPU6050 并打开运动中断
  * @retval 0：成功；1：读不到器件 ID
  */

u8 MPU_Init(void); 															//初始化MPU6050

/**
  * @brief  I2C 连续写若干个字节
  * @param  addr 7 位器件地址
  * @param  reg  起始寄存器
  * @param  len  字节数
  * @param  buf  数据缓冲区
  * @retval 0：成功；1：器件无应答
  */

u8 MPU_Write_Len(u8 addr,u8 reg,u8 len,u8 *buf);//IIC连续写
/**
  * @brief  I2C 连续读若干个字节
  * @param  addr 7 位器件地址
  * @param  reg  起始寄存器
  * @param  len  字节数
  * @param  buf  接收缓冲区
  * @retval 0：成功；1：器件无应答
  */

u8 MPU_Read_Len(u8 addr,u8 reg,u8 len,u8 *buf); //IIC连续读 
/**
  * @brief  I2C 写单个寄存器
  * @param  reg  寄存器地址
  * @param  data 要写的值
  * @retval 0：成功；1：器件无应答
  */

u8 MPU_Write_Byte(u8 reg,u8 data);							//IIC写一个字节
/**
  * @brief  I2C 读单个寄存器
  * @param  reg 寄存器地址
  * @retval 读到的寄存器值
  */

u8 MPU_Read_Byte(u8 reg);												//IIC读一个字节

/**
  * @brief  设置陀螺仪满量程
  * @param  fsr 0:+250dps 1:+500dps 2:+1000dps 3:+2000dps
  * @retval 0：成功；1：失败
  */

u8 MPU_Set_Gyro_Fsr(u8 fsr);
/**
  * @brief  设置加速度计满量程
  * @param  fsr 0:+2g 1:+4g 2:+8g 3:+16g
  * @retval 0：成功；1：失败
  */

u8 MPU_Set_Accel_Fsr(u8 fsr);
/**
  * @brief  设置数字低通滤波带宽
  * @param  lpf 期望带宽，单位 Hz
  * @retval 0：成功；1：失败
  */

u8 MPU_Set_LPF(u16 lpf);
/**
  * @brief  设置采样率
  * @param  rate 4~1000 Hz
  * @retval 0：成功；1：失败
  */

u8 MPU_Set_Rate(u16 rate);
/**
  * @brief  设置 FIFO（本工程没有实现，工程里搜不到定义）
  * @param  sens 传感器选择位
  * @retval 0：成功；1：失败
  */

u8 MPU_Set_Fifo(u8 sens);

/**
  * @brief  基于 iic_hal 的连续读封装
  * @param  addr   起始寄存器
  * @param  length 字节数
  * @param  buff   接收缓冲区
  * @retval 0：成功；1：失败
  */

uint8_t MPU_Read_Multi_Byte(uint8_t addr, uint8_t length, uint8_t buff[]);
/**
  * @brief  基于 iic_hal 的连续写封装
  * @param  addr   起始寄存器
  * @param  length 字节数
  * @param  buff   数据缓冲区
  * @retval 0：成功；1：失败
  */

uint8_t MPU_Write_Multi_Byte(uint8_t addr,uint8_t length,uint8_t buff[]);

/**
  * @brief  让 MPU6050 进入睡眠
  * @retval None
  */

void MPU_Sleep(void);
/**
  * @brief  唤醒 MPU6050
  * @retval None
  */

void MPU_Wakeup(void);
/**
  * @brief  读中断状态寄存器
  * @retval 寄存器原值
  */

uint8_t MPU_Read_Status(void);

/**
  * @brief  读片上温度
  * @retval 温度 x100 的整数
  */

short MPU_Get_Temperature(void);
/**
  * @brief  读三轴陀螺仪原始值
  * @param  gx gy gz 输出缓冲区
  * @retval 0：成功；1：读失败
  */

u8 MPU_Get_Gyroscope(short *gx,short *gy,short *gz);
/**
  * @brief  读三轴加速度原始值
  * @param  ax ay az 输出缓冲区
  * @retval 0：成功；1：读失败
  */

u8 MPU_Get_Accelerometer(short *ax,short *ay,short *az);
/**
  * @brief  由加速度算出 roll 和 pitch
  * @param  roll  输出，弧度
  * @param  pitch 输出，弧度
  * @retval None
  */

void MPU_Get_Angles(float * roll,float * pitch);
/**
  * @brief  判断手表是否水平
  * @retval 1：水平；0：不水平
  */

uint8_t MPU_isHorizontal(void);

#endif




































