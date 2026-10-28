/**
 * 硬件抽象层（Hardware Middle Layer）
 *
 * 对应参考实现：
 *   Software/OV_Watch/User/Func/Inc/HWDataAccess.h
 *
 * 目的只有一个：让界面代码不认识硬件。
 *
 *     界面（ui_HomePage.c）
 *            │  只认 HWInterface.xxx
 *            v
 *     HWDataAccess（本文件）
 *            │  编译期二选一
 *      ┌─────┴─────┐
 *   真实 BSP     仿真数据
 *   (实机)        (PC)
 *
 * 参考工程原话：加这个文件就是为了方便 UI 移植，把 MidFunc 和 UI 文件直接复制到
 * PC 仿真工程，再把下面的 HW_USE_HARDWARE 改成 0 就可以了。
 */
#ifndef HWDATAACCESS_H
#define HWDATAACCESS_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************
 *  硬件开关
 ***************************/
/*
 * 1 = 调用真实 BSP 驱动（实机，需要 rtc.h / power.h / MPU6050.h ...）
 * 0 = 全部返回仿真数据（PC，本工程现在用这个）
 *
 * 置 0 之后，HW_USE_RTC 等宏都不定义；下面 .c 里所有
 *     #if HW_USE_RTC
 * 都会走 #else 分支（未定义的宏在 #if 里按 0 处理）。
 */
#define HW_USE_HARDWARE 0

#if HW_USE_HARDWARE
  #define HW_USE_RTC    1
  #define HW_USE_BLE    1
  #define HW_USE_BAT    1
  #define HW_USE_LCD    1
  #define HW_USE_IMU    1
  #define HW_USE_AHT21  1
  #define HW_USE_SPL06  1
  #define HW_USE_LSM303 1
  #define HW_USE_EM7028 1
#endif

/***************************
 *  头文件
 ***************************/
#include <stdint.h>
#include <stdbool.h>

#if HW_USE_HARDWARE
  #include "rtc.h"
  #include "kt6328.h"
  #include "power.h"
  #include "lcd_init.h"
  #include "MPU6050.h"
  #include "inv_mpu.h"
  #include "inv_mpu_dmp_motion_driver.h"
  #include "AHT21.h"
  #include "SPL06_001.h"
  #include "LSM303.h"
  #include "em70x8.h"
#endif

/***************************
 *  类型定义
 ***************************/

/* 日期时间（字段都从 1 开始，Month 是 1-12，不是 BCD） */
typedef struct
{
    uint8_t WeekDay;    /* 1 = 周一 ... 7 = 周日 */
    uint8_t Month;      /* 1 - 12 */
    uint8_t Date;       /* 1 - 31 */
    uint8_t Year;       /* 0 - 99，即 2000 年之后 */
    uint8_t Hours;      /* 0 - 23 */
    uint8_t Minutes;    /* 0 - 59 */
    uint8_t Seconds;    /* 0 - 59 */
} HW_DateTimeTypeDef;

/* RTC：实时时钟 */
typedef struct
{
    void    (*GetTimeDate)(HW_DateTimeTypeDef *nowdatetime);
    void    (*SetDate)(uint8_t year, uint8_t month, uint8_t date);
    void    (*SetTime)(uint8_t hours, uint8_t minutes, uint8_t seconds);
    uint8_t (*CalculateWeekday)(uint8_t setyear, uint8_t setmonth, uint8_t setday, uint8_t century);
} HW_RTC_InterfaceTypeDef;

/* BLE：蓝牙模块 KT6328 */
typedef struct
{
    void (*Init)(void);
    void (*Enable)(void);
    void (*Disable)(void);
} HW_BLE_InterfaceTypeDef;

/* Power：电池电量 */
typedef struct
{
    uint8_t power_remain;               /* 0 - 100，由 BatCalculate() 刷新 */

    void    (*Init)(void);
    void    (*Shutdown)(void);
    uint8_t (*BatCalculate)(void);
} HW_Power_InterfaceTypeDef;

/* LCD：背光 */
typedef struct
{
    void (*SetLight)(uint8_t dc);       /* 0 - 100 */
} HW_LCD_InterfaceTypeDef;

/* IMU：MPU6050，负责计步和抬腕检测 */
#define WRIST_UP   1
#define WRIST_DOWN 0

typedef struct
{
    uint8_t  ConnectionError;           /* 0 = 正常，1 = 通信失败 */
    uint16_t Steps;
    uint8_t  wrist_state;
    uint8_t  wrist_is_enabled;

    uint8_t  (*Init)(void);
    void     (*WristEnable)(void);
    void     (*WristDisable)(void);
    uint16_t (*GetSteps)(void);
    int      (*SetSteps)(unsigned long count);
} HW_IMU_InterfaceTypeDef;

/* AHT21：温湿度 */
typedef struct
{
    uint8_t ConnectionError;
    uint8_t temperature;                /* 摄氏度，整数部分 */
    uint8_t humidity;                   /* 0 - 100 */
    uint8_t (*Init)(void);
    void    (*GetHumiTemp)(float *humi, float *temp);
} HW_AHT21_InterfaceTypeDef;

/* SPL06-001：气压计，算高度 */
typedef struct
{
    uint8_t  ConnectionError;
    uint16_t altitude;                  /* 米 */
    uint8_t  (*Init)(void);
} HW_Barometer_InterfaceTypeDef;

/* LSM303：电子罗盘 */
typedef struct
{
    uint8_t  ConnectionError;
    uint16_t direction;                 /* 0 - 359 度 */
    uint8_t  (*Init)(void);
    void     (*Sleep)(void);
} HW_Ecompass_InterfaceTypeDef;

/* EM7028：心率 / 血氧 */
typedef struct
{
    uint8_t ConnectionError;
    uint8_t HrRate;                     /* bpm */
    uint8_t SPO2;                       /* % */
    uint8_t (*Init)(void);
    void    (*Sleep)(void);
} HW_HRmeter_InterfaceTypeDef;

/* 所有硬件拼成一个结构体，整个程序只认这一个全局变量 */
typedef struct
{
    HW_RTC_InterfaceTypeDef     RealTimeClock;
    HW_BLE_InterfaceTypeDef     BLE;
    HW_Power_InterfaceTypeDef   Power;
    HW_LCD_InterfaceTypeDef     LCD;
    HW_IMU_InterfaceTypeDef     IMU;
    HW_AHT21_InterfaceTypeDef   AHT21;
    HW_Barometer_InterfaceTypeDef Barometer;
    HW_Ecompass_InterfaceTypeDef  Ecompass;
    HW_HRmeter_InterfaceTypeDef   HR_meter;
} HW_InterfaceTypeDef;

/***************************
 *  外部变量
 ***************************/
extern HW_InterfaceTypeDef HWInterface;

#if !HW_USE_HARDWARE
/*
 * PC 仿真专用，参考工程里没有这个函数。
 *
 * 实机上，结构体里的 Steps / power_remain / temperature 这些「字段」是由
 * 后台任务（IMUTask、PowerTask、AHT21Task）周期性地写进去的，界面只负责读。
 * PC 上还没有任务层，所以用这个函数代替：在 main 的循环里每秒调一次，
 * 把仿真数据写进 HWInterface。阶段 6 做任务层时它会消失。
 */
void HW_Sim_Update(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
