#ifndef __LCD_INIT_H
#define __LCD_INIT_H

#include "sys.h"

#define USE_HORIZONTAL 0  //设置横屏或者竖屏显示 0或1为竖屏 2或3为横屏


#if USE_HORIZONTAL==0||USE_HORIZONTAL==1
#define LCD_W 240
#define LCD_H 280

#else
#define LCD_W 280
#define LCD_H 240
#endif


//-----------------LCD端口定义---------------- 
#define SCLK_PORT			GPIOB
#define SCLK_PIN			GPIO_PIN_3

#define SDA_PORT			GPIOB
#define SDA_PIN				GPIO_PIN_5

#define RES_PORT			GPIOB
#define RES_PIN				GPIO_PIN_7

#define DC_PORT				GPIOB
#define DC_PIN				GPIO_PIN_9

#define CS_PORT				GPIOB
#define CS_PIN				GPIO_PIN_8

#define BLK_PORT			GPIOB
#define BLK_PIN				GPIO_PIN_0

#define LCD_SCLK_Clr() HAL_GPIO_WritePin(SCLK_PORT,SCLK_PIN,GPIO_PIN_RESET)//SCL=SCLK
#define LCD_SCLK_Set() HAL_GPIO_WritePin(SCLK_PORT,SCLK_PIN,GPIO_PIN_SET)

#define LCD_MOSI_Clr() HAL_GPIO_WritePin(SDA_PORT,SDA_PIN,GPIO_PIN_RESET)//SDA=MOSI
#define LCD_MOSI_Set() HAL_GPIO_WritePin(SDA_PORT,SDA_PIN,GPIO_PIN_SET)

#define LCD_RES_Clr()  HAL_GPIO_WritePin(RES_PORT,RES_PIN,GPIO_PIN_RESET)//RES
#define LCD_RES_Set()  HAL_GPIO_WritePin(RES_PORT,RES_PIN,GPIO_PIN_SET)

#define LCD_DC_Clr()   HAL_GPIO_WritePin(DC_PORT,DC_PIN,GPIO_PIN_RESET)//DC
#define LCD_DC_Set()   HAL_GPIO_WritePin(DC_PORT,DC_PIN,GPIO_PIN_SET)
 		     
#define LCD_CS_Clr()   HAL_GPIO_WritePin(CS_PORT,CS_PIN,GPIO_PIN_RESET)//CS
#define LCD_CS_Set()   HAL_GPIO_WritePin(CS_PORT,CS_PIN,GPIO_PIN_SET)

#define LCD_BLK_Clr()  HAL_GPIO_WritePin(BLK_PORT,BLK_PIN,GPIO_PIN_RESET)//BLK
#define LCD_BLK_Set()  HAL_GPIO_WritePin(BLK_PORT,BLK_PIN,GPIO_PIN_SET)

/**
  * @brief  初始化 LCD 控制脚 RES/CS/DC
  * @retval None
  */

void LCD_GPIO_Init(void);//初始化GPIO
/**
  * @brief  通过 SPI1 发送一个字节
  * @param  dat 要发送的字节
  * @retval None
  */

void LCD_Writ_Bus(u8 dat);//模拟SPI时序
/**
  * @brief  写一个字节数据
  * @param  dat 数据字节
  * @retval None
  */

void LCD_WR_DATA8(u8 dat);//写入一个字节
/**
  * @brief  写一个 16 位数据（高字节在前）
  * @param  dat 16 位数据
  * @retval None
  */

void LCD_WR_DATA(u16 dat);//写入两个字节
/**
  * @brief  写一条命令（发送期间 DC 拉低）
  * @param  dat 命令码
  * @retval None
  */

void LCD_WR_REG(u8 dat);//写入一个指令
/**
  * @brief  设置写入窗口（0x2A/0x2B/0x2C）
  * @param  x1 y1 左上角
  * @param  x2 y2 右下角
  * @retval None
  */

void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2);//设置坐标函数
/**
  * @brief  ST7789 上电初始化
  * @retval None
  */

void LCD_Init(void);//LCD初始化
/**
  * @brief  设置背光亮度
  * @param  dc 占空比 5~100，单位百分比
  * @retval None
  */

void LCD_Set_Light(uint8_t dc);
/**
  * @brief  关闭背光
  * @retval None
  */

void LCD_Close_Light(void);
/**
  * @brief  让 ST7789 睡眠
  * @retval None
  */

void LCD_ST7789_SleepIn(void);
/**
  * @brief  唤醒 ST7789
  * @retval None
  */

void LCD_ST7789_SleepOut(void);
/**
  * @brief  启动背光 PWM
  * @retval None
  */

void LCD_Open_Light(void);
#endif




