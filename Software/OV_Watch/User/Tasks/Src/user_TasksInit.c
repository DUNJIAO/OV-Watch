/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
//sys
#include "sys.h"
#include "stdio.h"
#include "lcd.h"
#include "WDOG.h"
//gui
#include "lvgl.h"
#include "ui_TimerPage.h"
//tasks
#include "user_HardwareInitTask.h"
#include "user_RunModeTasks.h"
#include "user_KeyTask.h"
#include "user_ScrRenewTask.h"
#include "user_SensUpdateTask.h"
#include "user_ChargCheckTask.h"
#include "user_MessageSendTask.h"
#include "user_DataSaveTask.h"

/* Private typedef -----------------------------------------------------------*/


/* Private define ------------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/


/*
 * 任务层的总入口。本文件负责：
 *   1. 创建 6 个消息队列 + 1 个事件标志
 *   2. 创建 13 个任务
 *   3. 提供 SysTick 钩子 TaskTickHook（跑在中断里）
 *
 * 优先级说明：osPriority 数值越大越紧急
 *   High3 > High2 > High1 > High > Normal > Low2 > Low1 > Low
 *   注意 Low2(10) 比 Low1(9) 高，别被名字顺序骗了
 *
 * 栈大小：CMSIS-RTOS2 的 stack_size 单位是【字节】，
 *   内部按 stack_size / 4 换算成 FreeRTOS 的字数
 *
 * 13 个任务一览：
 *   HardwareInitTask    初始化全部硬件，失败重试 3 次，跑完自己退出
 *   WDOGFeedTask        每 100 ms 喂一次外部看门狗
 *   IdleEnterTask       熄屏管理：闲置到"暗屏时间"就调暗背光，有触摸或按键就恢复；
 *                       再超时到"熄屏时间"就发 Stop 信号
 *   StopEnterTask       收到 Stop 信号后真正进入 STOP 低功耗，醒来后恢复时钟并检查抬腕
 *   KeyTask             调 KeyScan(0) 拿按键，投递到 Key_MessageQueue；
 *                       在主页按 KEY2 直接发 Stop 信号而不是返回
 *   ScrRenewTask        消费按键和主页刷新信号，切页面
 *   SensorDataTask      收到主页刷新信号时采集电量/步数/温湿度，发布给界面
 *   HRDataTask          只在心率页采集心率，不在心率页就把传感器关掉省电
 *   ChargPageEnterTask  检测充电器插拔，切到充电页面
 *   MessageSendTask     通过 KT6328 蓝牙发送消息
 *   MPUCheckTask        每 300 ms 查一次姿态，手腕放下且停在主页/菜单/设置时发 Stop 信号
 *   DataSaveTask        把设置和计步写进 EEPROM
 *   LvHandlerTask       跑 LVGL 主循环，全场优先级最低
 * ---------------------------------------------------------------------------*/

/* Tasks ---------------------------------------------------------------------*/
// Hardwares initialization
osThreadId_t HardwareInitTaskHandle;
const osThreadAttr_t HardwareInitTask_attributes = {
  .name = "HardwareInitTask",
  .stack_size = 128 * 10,
  .priority = (osPriority_t) osPriorityHigh3,
};

//LVGL Handler task
osThreadId_t LvHandlerTaskHandle;
const osThreadAttr_t LvHandlerTask_attributes = {
  .name = "LvHandlerTask",
  .stack_size = 128 * 24,
  .priority = (osPriority_t) osPriorityLow,
};

//WDOG Feed task
osThreadId_t WDOGFeedTaskHandle;
const osThreadAttr_t WDOGFeedTask_attributes = {
  .name = "WDOGFeedTask",
  .stack_size = 128 * 1,
  .priority = (osPriority_t) osPriorityHigh2,
};

//Idle Enter Task
osThreadId_t IdleEnterTaskHandle;
const osThreadAttr_t IdleEnterTask_attributes = {
  .name = "IdleEnterTask",
  .stack_size = 128 * 1,
  .priority = (osPriority_t) osPriorityHigh,
};

//Stop Enter Task
osThreadId_t StopEnterTaskHandle;
const osThreadAttr_t StopEnterTask_attributes = {
  .name = "StopEnterTask",
  .stack_size = 128 * 16,
  .priority = (osPriority_t) osPriorityHigh1,
};

//Key task
osThreadId_t KeyTaskHandle;
const osThreadAttr_t KeyTask_attributes = {
  .name = "KeyTask",
  .stack_size = 128 * 1,
  .priority = (osPriority_t) osPriorityNormal,
};

//ScrRenew task
osThreadId_t ScrRenewTaskHandle;
const osThreadAttr_t ScrRenewTask_attributes = {
  .name = "ScrRenewTask",
  .stack_size = 128 * 10,
  .priority = (osPriority_t) osPriorityLow1,
};

//SensorDataRenew task
osThreadId_t SensorDataTaskHandle;
const osThreadAttr_t SensorDataTask_attributes = {
  .name = "SensorDataTask",
  .stack_size = 128 * 5,
  .priority = (osPriority_t) osPriorityLow1,
};

//HRDataRenew task
osThreadId_t HRDataTaskHandle;
const osThreadAttr_t HRDataTask_attributes = {
  .name = "HRDataTask",
  .stack_size = 128 * 5,
  .priority = (osPriority_t) osPriorityLow1,
};

//ChargPageEnterTask
osThreadId_t ChargPageEnterTaskHandle;
const osThreadAttr_t ChargPageEnterTask_attributes = {
  .name = "ChargPageEnterTask",
  .stack_size = 128 * 10,
  .priority = (osPriority_t) osPriorityLow1,
};

//messagesendtask
osThreadId_t MessageSendTaskHandle;
const osThreadAttr_t MessageSendTask_attributes = {
  .name = "MessageSendTask",
  .stack_size = 128 * 5,
  .priority = (osPriority_t) osPriorityLow1,
};

//MPUCheckTask
osThreadId_t MPUCheckTaskHandle;
const osThreadAttr_t MPUCheckTask_attributes = {
  .name = "MPUCheckTask",
  .stack_size = 128 * 3,
  .priority = (osPriority_t) osPriorityLow2,
};

//DataSaveTask
osThreadId_t DataSaveTaskHandle;
const osThreadAttr_t DataSaveTask_attributes = {
  .name = "DataSaveTask",
  .stack_size = 128 * 5,
  .priority = (osPriority_t) osPriorityLow2,
};


/* Message queues ------------------------------------------------------------*/
//Key message
osMessageQueueId_t Key_MessageQueue;
osMessageQueueId_t Idle_MessageQueue;
osMessageQueueId_t Stop_MessageQueue;
osMessageQueueId_t IdleBreak_MessageQueue;
osMessageQueueId_t HomeUpdata_MessageQueue;
osMessageQueueId_t DataSave_MessageQueue;
osEventFlagsId_t HardIntEventHandle;

/* Private function prototypes -----------------------------------------------*/
void LvHandlerTask(void *argument);
void WDOGFeedTask(void *argument);

/**
  * @brief  创建所有消息队列、事件标志和任务
  * @param  None
  * @retval None
  * @note   由 CubeMX 生成的 freertos.c 里的 MX_FREERTOS_Init 调用，
  *         必须在内核启动之前执行（osKernelStart 之前）
  *         队列元素长度都是 1 字节，只是用来传"有事件发生"这个信号；
  *         Key_MessageQueue 存的是按键编号 1 或 2
  *         最后往 HomeUpdata_MessageQueue 里塞了一个初值，
  *         让界面启动后立刻刷新一次主页
  */
void User_Tasks_Init(void)
{
  /* add mutexes, ... */

  /* add semaphores, ... */

  /* start timers, add new ones, ... */

  /* add queues, ... */
	Key_MessageQueue  = osMessageQueueNew(1, 1, NULL);
	Idle_MessageQueue = osMessageQueueNew(1, 1, NULL);
	Stop_MessageQueue = osMessageQueueNew(1, 1, NULL);
	IdleBreak_MessageQueue = osMessageQueueNew(1, 1, NULL);
	HomeUpdata_MessageQueue = osMessageQueueNew(1, 1, NULL);
	DataSave_MessageQueue = osMessageQueueNew(2, 1, NULL);
  HardIntEventHandle = osEventFlagsNew(NULL);

	/* add threads, ... */
  HardwareInitTaskHandle  = osThreadNew(HardwareInitTask, NULL, &HardwareInitTask_attributes);
  LvHandlerTaskHandle  = osThreadNew(LvHandlerTask, NULL, &LvHandlerTask_attributes);
  WDOGFeedTaskHandle   = osThreadNew(WDOGFeedTask, NULL, &WDOGFeedTask_attributes);
	IdleEnterTaskHandle  = osThreadNew(IdleEnterTask, NULL, &IdleEnterTask_attributes);
	StopEnterTaskHandle  = osThreadNew(StopEnterTask, NULL, &StopEnterTask_attributes);
	KeyTaskHandle 			 = osThreadNew(KeyTask, NULL, &KeyTask_attributes);
	ScrRenewTaskHandle   = osThreadNew(ScrRenewTask, NULL, &ScrRenewTask_attributes);
	SensorDataTaskHandle = osThreadNew(SensorDataUpdateTask, NULL, &SensorDataTask_attributes);
	HRDataTaskHandle		 = osThreadNew(HRDataUpdateTask, NULL, &HRDataTask_attributes);
	ChargPageEnterTaskHandle = osThreadNew(ChargPageEnterTask, NULL, &ChargPageEnterTask_attributes);
  MessageSendTaskHandle = osThreadNew(MessageSendTask, NULL, &MessageSendTask_attributes);
	MPUCheckTaskHandle		= osThreadNew(MPUCheckTask, NULL, &MPUCheckTask_attributes);
	DataSaveTaskHandle		= osThreadNew(DataSaveTask, NULL, &DataSaveTask_attributes);

  /* add events, ... */


	/* add  others ... */
	uint8_t HomeUpdataStr;
	osMessageQueuePut(HomeUpdata_MessageQueue, &HomeUpdataStr, 0, 1);

}


/**
  * @brief  FreeRTOS Tick Hook, to increase the LVGL tick
  * @param  None
  * @retval None
  * @note   跑在 SysTick 中断里，每 1 ms 进一次，所以这里【不能】调用 osDelay 等阻塞接口
  *         一次干四件事：
  *           1. lv_tick_inc(1)          给 LVGL 喂毫秒时基
  *           2. IdleTimerCount++        不在计时页时累加闲置时间，用于熄屏
  *           3. ui_TimerPage_elapsed_ms 计时页在跑时累加秒表
  *           4. user_HR_timecount++     给心率算法当毫秒时钟
  */
void TaskTickHook(void)
{
	//to increase the LVGL tick
	lv_tick_inc(1);
	//to increase the timerpage's timer(put in here is to ensure the Real Time)
	if(ui_TimerPageFlag)
	{
    IdleTimerCount = 0; //闲置时间
    ui_TimerPage_elapsed_ms += 1;
	}
  else
  {
    IdleTimerCount += 1;  //没在计时，闲置计数+1，用于熄屏
  }
	user_HR_timecount+=1;  //给心率算法当毫秒时钟
}


/**
  * @brief  LVGL Handler task, to run the lvgl
  * @param  argument: Not used
  * @retval None
  * @note   全场优先级最低（osPriorityLow），界面抢不到 CPU 时会最后才被调度；
  *         每轮先看触摸有没有超过 1 秒没动作，有动作就往 IdleBreak_MessageQueue 放信号，
  *         阻止熄屏；然后跑 lv_task_handler，它返回的等待时间被夹在 1~30 ms：
  *           夹下限 1 ms，避免 osDelay(0) 只让出 CPU 不睡眠导致空转
  *           夹上限 30 ms，保证界面刷新不卡顿
  */
void LvHandlerTask(void *argument)
{
	uint8_t IdleBreakstr=0;
  while(1)
  {
		if(lv_disp_get_inactive_time(NULL)<1000)
		{
			//Idle time break, set to 0
			osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 0);
		}
    uint32_t wait = lv_task_handler();
    if(wait < 1U) wait = 1U;
    if(wait > 30U) wait = 30U;
    osDelay(wait);
	}
}


/**
  * @brief  Watch Dog Feed task
  * @param  argument: Not used
  * @retval None
  * @note   每 100 ms 翻转一次 WDI 脚，把外部看门狗喂住；
  *         注意顺序是先 Feed 再 Enable，第一轮循环时看门狗还没打开，
  *         所以真正生效是从第二轮开始（实际影响可以忽略）
  */
void WDOGFeedTask(void *argument)
{
	//owdg
	WDOG_Port_Init();
  while(1)
  {
		WDOG_Feed();
		WDOG_Enable();
    osDelay(100);
  }
}


