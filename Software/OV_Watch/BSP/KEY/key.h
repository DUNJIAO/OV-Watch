#ifndef __KEY_H__
#define __KEY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

//KEY1
#define KEY1_PORT	GPIOA
#define KEY1_PIN	GPIO_PIN_5
#define KEY1 HAL_GPIO_ReadPin(KEY1_PORT,KEY1_PIN)
//KEY_Wake  
#define KEY2_PORT	GPIOA
#define KEY2_PIN	GPIO_PIN_4
#define KEY2 HAL_GPIO_ReadPin(KEY2_PORT,KEY2_PIN)
	
//初始化按键，配置按键
void Key_Port_Init(void);

//
void Key_Interrupt_Callback(void);

//按键扫描函数
//mode=1表示初始化按键状态，mode=0表示正常扫描按键 
uint8_t KeyScan(uint8_t mode);




#ifdef __cplusplus
}
#endif
#endif

