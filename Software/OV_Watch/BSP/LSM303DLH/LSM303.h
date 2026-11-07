#ifndef _LSM303_H_
#define _LSM303_H_

#include "iic_hal.h"

/*
 * LSM303DLH：一个封装里有两个独立器件
 *   加速度计   地址 0x19，寄存器 0x20~0x37
 *   磁力计/温度 地址 0x1E，寄存器 0x00~0x0C 和 0x31~0x32
 * 两者共用 PB13（SDA）/ PB14（SCL）
 * 注意：这颗器件不支持连续读，多字节要逐字节单独读
 */

//LSM303DLH 寄存器定义
//罗盘
#define	LSM303_CRA_REG_M					0x00	//RW
#define	LSM303_CRB_REG_M					0x01	//RW
#define	LSM303_MR_REG_M						0x02	//RW
#define	LSM303_OUT_X_H_M					0x03	//R
#define	LSM303_OUT_X_L_M					0x04	//R
#define	LSM303_OUT_Z_H_M					0x05	//R
#define	LSM303_OUT_Z_L_M					0x06	//R
#define	LSM303_OUT_Y_H_M					0x07	//R
#define	LSM303_OUT_Y_L_M					0x08	//R
#define	LSM303_SR_REG_M						0x09	//R
#define	LSM303_IRA_REG_M					0x0A	//R
#define	LSM303_IRB_REG_M					0x0B	//R
#define	LSM303_IRC_REG_M					0x0C	//R

//加速度
#define	LSM303_CTRL_REG1_A					0x20	//RW
#define	LSM303_CTRL_REG2_A					0x21	//RW
#define	LSM303_CTRL_REG3_A					0x22	//RW
#define	LSM303_CTRL_REG4_A					0x23	//RW
#define	LSM303_CTRL_REG5_A					0x24	//RW
#define	LSM303_HP_FILTER_RESET_A		0x25	//R
#define	LSM303_REFERENCE_A					0x26	//RW
#define	LSM303_STATUS_REG_A					0x27	//R
#define	LSM303_OUT_X_L_A						0x28	//R
#define	LSM303_OUT_X_H_A						0x29	//R
#define	LSM303_OUT_Y_L_A						0x2A	//R
#define	LSM303_OUT_Y_H_A						0x2B	//R
#define	LSM303_OUT_Z_L_A						0x2C	//R
#define	LSM303_OUT_Z_H_A						0x2D	//R
#define	LSM303_INT1_CFG_A						0x30	//RW
#define	LSM303_INT1_SOURCE_A				0x31	//R
#define	LSM303_INT1_THS_A						0x32	//RW
#define	LSM303_INT1_DURATION_A			0x33	//RW
#define	LSM303_INT2_CFG_A						0x34	//RW
#define	LSM303_INT2_SOURCE_A				0x35	//R
#define	LSM303_INT2_THS_A						0x36	//RW
#define	LSM303_INT2_DURATION_A			0x37	//RW

//温度
#define TEMP_OUT_H_M								0x31	//R
#define TEMP_OUT_L_M								0x32	//R

#define LSM303_SlaveAddr_A					0x19	//加速度地址
#define LSM303_SlaveAddr_M					0x1E	//磁力计和温度的地址

//functions

/**
  * @brief  读一个寄存器
  * @param  RegAddr 寄存器地址
  * @retval 读到的字节
  * @note   按地址自动选从机：> 0x19 走加速度，否则走磁力计
  */
unsigned char LSM303_ReadOneReg(unsigned char RegAddr);

/**
  * @brief  连续读多个相邻寄存器
  * @param  RegAddr 起始寄存器地址
  * @param  RegNum  要读的字节数
  * @param  DataBuff 接收缓冲区，由调用方准备
  * @retval None
  * @note   这颗器件不支持连续读，内部是循环调用单字节读，每个字节一次完整传输
  */
void LSM303_ReadMultiReg(unsigned char RegAddr, unsigned char RegNum, unsigned char DataBuff[]);

/**
  * @brief  读温度寄存器
  * @param  RegAddr 寄存器地址
  * @retval 读到的字节
  * @note   固定走磁力计地址 0x1E，温度寄存器挂在磁力计这一侧
  */
unsigned char LSM303_Temp_ReadOneReg(unsigned char RegAddr);

/**
  * @brief  写一个寄存器
  * @param  RegAddr 寄存器地址
  * @param  dat     要写入的字节
  * @retval None
  * @note   按地址自动选从机，规则同 ReadOneReg
  */
void LSM303_WriteOneReg(unsigned char RegAddr, unsigned char dat);

/**
  * @brief  初始化加速度计和磁力计，写入配置后回读校验
  * @retval 0：成功；1：加速度或磁力计配置没写进去
  */
unsigned char LSM303DLH_Init(void);

/**
  * @brief  两个器件都进入低功耗
  * @retval None
  */
void LSM303DLH_Sleep(void);

/**
  * @brief  恢复连续测量
  * @retval None
  */
void LSM303DLH_Wakeup(void);

/**
  * @brief  读三轴加速度
  * @param  Xa,Ya,Za 输出参数，原始 LSB 值
  * @retval None
  * @note   要换算成 g（重力加速度），还需按量程除以对应灵敏度
  */
void LSM303_ReadAcceleration(int16_t *Xa, int16_t *Ya, int16_t *Za);

/**
  * @brief  读三轴磁场
  * @param  Xm,Ym,Zm 输出参数，原始 LSB 值
  * @retval None
  * @note   寄存器排列是 X、Z、Y，不是 X、Y、Z，函数内部已调整
  */
void LSM303_ReadMagnetic(int16_t *Xm, int16_t *Ym, int16_t *Zm);

/**
  * @brief  计算 Z 轴倾角
  * @param  Xa,Ya,Za 三轴加速度原始值
  * @retval 角度 × 100，单位 0.01 度
  */
int LSM303DLH_CalculationZAxisAngle(int16_t Xa, int16_t Ya, int16_t Za);

/**
  * @brief  计算 X 轴倾角
  * @param  Xa,Ya,Za 三轴加速度原始值
  * @retval 角度 × 100，单位 0.01 度
  */
int LSM303DLH_CalculationXAxisAngle(int16_t Xa, int16_t Ya, int16_t Za);

/**
  * @brief  读温度
  * @param  Temp 输出参数，摄氏度
  * @retval None
  */
void LSM303_ReadTemperature(int16_t *Temp);

/**
  * @brief  由加速度和磁场算方位角
  * @param  Xa,Ya,Za 三轴加速度；Xm,Ym,Zm 三轴磁场
  * @retval 方位角，单位 度
  * @note   先用加速度算出俯仰角和横滚角做倾斜补偿，再算磁方位
  */
float Azimuth_Calculate(int16_t Xa, int16_t Ya, int16_t Za, int16_t Xm, int16_t Ym, int16_t Zm);

#endif /*_LSM303DLH_H_*/
