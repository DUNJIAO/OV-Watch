/* Private includes -----------------------------------------------------------*/
//includes
#include "user_TasksInit.h"
#include "user_ScrRenewTask.h"
#include "main.h"
#include "lvgl.h"
#include "ui_HomePage.h"
#include "ui_MenuPage.h"


/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
extern osMessageQueueId_t Key_MessageQueue;

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  Screen renew task
  * @param  argument: Not used
  * @retval None
  * @note   这是 KeyTask 的消费者：KeyTask 每 1 ms 扫键并投递，本任务每 10 ms 取一次。
  *         取队列超时填 0（不阻塞），循环末尾 osDelay(10)，所以按键响应延迟最多约 10 ms。
  *         KEY1 -> Page_Back() 退一页；退完后若落在主菜单 ui_MenuPage，
  *                 关掉 EM7028 心率与 LSM303 地磁（省电，再进测量页才会重新打开）
  *         KEY2 -> Page_Back_Bottom() 直接回首页，必然离开测量页，故无条件关这两个传感器
  *         注意：先 Page_Back() 再取 CurrentPage，判断的是【退完之后】的页面
  */
void ScrRenewTask(void *argument)
{
	uint8_t keystr=0;
	while(1)
	{
		//osMessageQueueGet（要取的队列，存储位置，超时时间ms,等待时间ms）
		if(osMessageQueueGet(Key_MessageQueue,&keystr,NULL,0)==osOK)
		{
			//key1 pressed
			if(keystr == 1)
			{

				Page_Back();
				if(Page_Get_NowPage()->page_obj == &ui_MenuPage)
				{
					//HR sensor sleep
					EM7028_hrs_DisEnable();
					//sensor sleep
					LSM303DLH_Sleep();
					// SPL_Sleep();
				}
			}
			//key2 pressed
			else if(keystr == 2)
			{
				Page_Back_Bottom();
				//HR sensor sleep
				EM7028_hrs_DisEnable();
  				//sensor sleep
				LSM303DLH_Sleep();
				// SPL_Sleep();
			}
		}
		osDelay(10);
	}
}
