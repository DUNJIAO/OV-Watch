/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"

#include "ui.h"
#include "ui_HomePage.h"
#include "ui_OffTimePage.h"
#include "ui_TimerPage.h"

#include "main.h"
#include "stm32f4xx_it.h"
#include "usart.h"
#include "lcd_init.h"
#include "power.h"
#include "CST816.h"
#include "MPU6050.h"
#include "key.h"
#include "WDOG.h"

#include "HWDataAccess.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint32_t IdleTimerCount = 0;

/* Private function prototypes -----------------------------------------------*/

/* Tasks ---------------------------------------------------------------------*/

/**
	* @brief  Enter Idle state
	* @param  argument: Not used
	* @retval None
	* @note   每 10 ms 跑一轮，管"变暗 -> 熄屏"这一整条时间链：
	*           收到 Idle_MessageQueue   -> 背光调到 5（只变暗，不关屏）
	*           收到 IdleBreak_MessageQueue -> 恢复用户设定的亮度，闲置计时清零
	*           IdleTimerCount >= ui_LTimeValue*1000 -> 自己给自己发 Idle 信号
	*           IdleTimerCount >= ui_TTimeValue*1000 -> 给 StopEnterTask 发 Stop 信号
	*         light_off_sent 是防重复标记：变暗后就不再反复发 Idle 信号
	*         IdleTimerCount 由 SysTick 钩子 TaskTickHook 每 1 ms 加 1
	*/
void IdleEnterTask(void *argument)
{
	uint8_t Idlestr=0;
	uint8_t IdleBreakstr=0;
	uint8_t light_off_sent = 0;
	while(1)
	{
		//light get dark
		if(osMessageQueueGet(Idle_MessageQueue,&Idlestr,NULL,1)==osOK)
		{
			LCD_Set_Light(5);
			light_off_sent = 1;
		}
		//resume light if light got dark and idle state breaked by key pressing or screen touching
		if(osMessageQueueGet(IdleBreak_MessageQueue,&IdleBreakstr,NULL,1)==osOK)
		{
			IdleTimerCount = 0;
			light_off_sent = 0;
			LCD_Set_Light(ui_LightSliderValue);
		}

		if((!light_off_sent) && (IdleTimerCount >= ((uint32_t)ui_LTimeValue * 1000U)))
		{
			uint8_t IdleMsg = 0;
			osMessageQueuePut(Idle_MessageQueue, &IdleMsg, 0, 0);
		}

		if(IdleTimerCount >= ((uint32_t)ui_TTimeValue * 1000U))
		{
			uint8_t Stopstr = 1;
			IdleTimerCount = 0;
			light_off_sent = 0;
			osMessageQueuePut(Stop_MessageQueue, &Stopstr, 0, 0);
		}

		osDelay(10);
	}
}

/**
  * @brief  enter the stop mode and resume
  * @param  argument: Not used
  * @retval None
  * @note   分工：IdleEnterTask 负责"判断该睡了"，本任务负责"真正睡下去、再醒过来"
  *         睡前三件事：关外设（UART/LCD/触摸）、挂起调度器、关看门狗和 SysTick 中断
  *         醒来后必须重做三件事：恢复 SysTick、SystemClock_Config（STOP 会把时钟切回
  *         内部 16 MHz HSI，不配置就按 16 MHz 跑）、喂狗
  *         sleep 标签 + goto 是核心：醒来若没按键、没充电、没抬腕，直接 goto 回去再睡
  *         队列用 timeout=0 取，靠末尾 osDelay(100) 轮询，所以响应最慢约 100 ms
  */
void StopEnterTask(void *argument)
{
	uint8_t Stopstr;
	uint8_t HomeUpdataStr;
	uint8_t Wrist_Flag=0;
	while(1)
	{
		if(osMessageQueueGet(Stop_MessageQueue,&Stopstr,NULL,0)==osOK)
		{

			/*************************** your operations before sleep***************************/
			sleep:
			IdleTimerCount = 0;

			//sensors

			//usart
			HAL_UART_MspDeInit(&huart1);

			//lcd
			LCD_RES_Clr();
			LCD_Close_Light();
			//touch
			CST816_Sleep();

			/***********************************************************************************/

			/****************************** enter wakeup operations *****************************/

			vTaskSuspendAll();
			//Disnable Watch Dog
			WDOG_Disnable();
			//systick int
			CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
			//enter stop mode
			HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON,PWR_STOPENTRY_WFI);

			//here is the sleep period

			/***********************************************************************************/

			/****************************** quit wakeup operations *****************************/

			//resume run mode and reset the sysclk
			SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
			HAL_SYSTICK_Config(SystemCoreClock / (1000U / uwTickFreq));
			SystemClock_Config();
			WDOG_Feed();
			xTaskResumeAll();

			/***********************************************************************************/

			/****************************** your wakeup operations *****************************/

			//MPU Check
			if(HWInterface.IMU.wrist_is_enabled)
			{
				uint8_t hor;
				hor = MPU_isHorizontal();
				if(hor && HWInterface.IMU.wrist_state == WRIST_DOWN)
				{
					HWInterface.IMU.wrist_state = WRIST_UP;
					Wrist_Flag = 1;
					//resume, go on
				}
				else if(!hor && HWInterface.IMU.wrist_state == WRIST_UP)
				{
					HWInterface.IMU.wrist_state = WRIST_DOWN;
					IdleTimerCount  = 0;
					goto sleep;
				}
			}

			//
			if(!KEY1 || KEY2 || ChargeCheck() || Wrist_Flag)
			{
				Wrist_Flag = 0;
				//resume, go on
			}
			else
			{
				IdleTimerCount  = 0;
				goto sleep;
			}

			//usart
			HAL_UART_MspInit(&huart1);
			//lcd
			LCD_Init();
			LCD_Set_Light(ui_LightSliderValue);
			//touch
			CST816_Wakeup();
			//check if is Charging
			if(ChargeCheck())
			{osEventFlagsSet(HardIntEventHandle, HARDINT_EVENT_CHARG);}
			//send the Home Updata message
			osMessageQueuePut(HomeUpdata_MessageQueue, &HomeUpdataStr, 0, 1);

			/**************************************************************************************/

		}
		osDelay(100);
	}
}

