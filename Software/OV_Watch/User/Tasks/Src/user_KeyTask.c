/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "ui_HomePage.h"
#include "main.h"
#include "key.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  Key press check task
  * @param  argument: Not used
  * @retval None
  * @note   每 1 ms 调一次 KeyScan(0)，KeyScan 只在【松手】那一刻返回 1 或 2，其余时间返回 0
  *         KEY1：投递给 ScrRenewTask（页面返回），同时解除熄屏状态
  *         KEY2：在主页时直接发 Stop 信号去睡觉；不在主页时投递给 ScrRenewTask（回到底页）
  *         队列容量只有 1，Put 超时 1 ms：连按太快时后一个按键会被丢弃
  */
void KeyTask(void *argument)
{
	uint8_t keystr=0;
	uint8_t Stopstr=0;
	uint8_t IdleBreakstr=0;
	while(1)
	{
		switch(KeyScan(0))
		{
			case 1:
				keystr = 1;
				osMessageQueuePut(Key_MessageQueue, &keystr, 0, 1);
				osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
				break;

			case 2:
				if(Page_Get_NowPage()->page_obj == &ui_HomePage)
				{
					osMessageQueuePut(Stop_MessageQueue, &Stopstr, 0, 1);
				}
				else
				{
					keystr = 2;
					osMessageQueuePut(Key_MessageQueue, &keystr, 0, 1);
					osMessageQueuePut(IdleBreak_MessageQueue, &IdleBreakstr, 0, 1);
				}
				break;
		}
		osDelay(1);
	}
}
