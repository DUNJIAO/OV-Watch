/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "user_ScrRenewTask.h"
#include "user_RunModeTasks.h"
#include "ui_HomePage.h"
#include "ui_ChargPage.h"
#include "main.h"
#include "HWDataAccess.h"
#include "stm32f4xx_it.h"
/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  charg page enter task
  * @param  argument: Not used
  * @retval None
  * @note   阻塞在 HardIntEventHandle 的 HARDINT_EVENT_CHARG 上（osWaitForever），
  *         平时完全不占 CPU。标志位由两处置位：
  *           a) PA2（CHARGE_PIN）双边沿 EXTI2 中断，见 stm32f4xx_it.c:277，
  *              插拔充电器都触发；NVIC 优先级 5，满足 FreeRTOS 的
  *              configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 要求，可在 ISR 里调 osEventFlagsSet
  *           b) IdleEnterTask 从 STOP 唤醒时主动查一次 ChargeCheck() 再补发，
  *              见 user_RunModeTasks.c:187
  *         这是"中断只置标志、任务做实事"的典型写法。
  *         醒来先往 IdleBreak_MessageQueue 投一条空消息，让 IdleEnterTask 恢复亮度、
  *         清零闲置计时（插拔充电器算一次用户活动，不能熄屏）。
  *         随后按 ChargeCheck() 的真实状态切页，两个分支都带"当前页面"判断，互斥：
  *           在充电 且 不在充电页 -> Page_Load(&Page_Charg)
  *           不在充电 且 在充电页 -> Page_Back()
  *         这样即使插拔抖动导致标志重复置位，也不会反复加载同一个页面。
  */
void ChargPageEnterTask(void *argument)
{
	while(1)
	{
		uint32_t hardint_flags = osEventFlagsWait(HardIntEventHandle, HARDINT_EVENT_CHARG, osFlagsWaitAny, osWaitForever);
		if((hardint_flags & HARDINT_EVENT_CHARG) != 0U)
		{
			uint8_t IdleBreakstr = 0;
			osMessageQueuePut(IdleBreak_MessageQueue,&IdleBreakstr,NULL,1);
			if((ChargeCheck()) && (Page_Get_NowPage()->page_obj != &ui_ChargPage))
			{
				Page_Load(&Page_Charg);
			}
			else if((!ChargeCheck()) && (Page_Get_NowPage()->page_obj == &ui_ChargPage))
			{
				Page_Back();
			}
		}
	}
}

