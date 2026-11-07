/* Private includes -----------------------------------------------------------*/

// includes
// sys
#include "usart.h"
#include "tim.h"
#include "stm32f4xx_it.h"
#include "delay.h"

// user
#include "user_TasksInit.h"
#include "HWDataAccess.h"
#include "version.h"

// bsp
#include "key.h"
#include "lcd.h"
#include "lcd_init.h"
#include "CST816.h"
#include "DataSave.h"

// ui
//gui
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "ui.h"

// APP SYS setting
#include "ui_DateTimeSetPage.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/


/**
  * @brief  hardwares init task
  * @param  argument: Not used
  * @retval None
  * @note   整个工程的上电初始化，只跑一遍，干完自己删除：
  *           1. vTaskSuspendAll() 冻结其余 12 个任务，防止初始化过程中被抢 CPU
  *           2. RTC 唤醒定时器、UART DMA 接收、背光 PWM
  *           3. 电源、按键
  *           4. 5 个传感器，每个最多重试 3 次，结果记进 ConnectionError
  *           5. EEPROM 校验，恢复手臂唤醒开关、APP 开关、当天步数
  *           6. 蓝牙、触摸、LCD，显示 Welcome 和版本号（阻塞 1 秒）
  *           7. LVGL 三件套：lv_init -> lv_port_disp_init -> lv_port_indev_init -> ui_init
  *           8. xTaskResumeAll() 解冻，vTaskDelete(NULL) 删除自己
  *         期间大量使用 delay_ms，它是纯 SysTick 忙等（delay.c 里 OS_SUPPORT=0），
  *         不依赖调度器，所以调度器挂起时照样能用，代价是空转 CPU
  *         末尾的 osDelay(500) 永远执行不到，因为前面已经把自己删了
  */
void HardwareInitTask(void *argument)
{
	while(1)
	{
    vTaskSuspendAll();//冻结其他12个任务，防止初始化过程中被抢 CPU

    // RTC Wake
    if(HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 2000, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK)
    {
      Error_Handler();
    }
    // usart start
    HAL_UART_Receive_DMA(&huart1,(uint8_t*)HardInt_receive_str,25);
    __HAL_UART_ENABLE_IT(&huart1,UART_IT_IDLE);

    // PWM Start
    HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_3);

    // sys delay
    delay_init();
    // wait
    // delay_ms(1000);

    // power
    HWInterface.Power.Init();

    // key
    Key_Port_Init();

    // sensors
    uint8_t num = 3;
    while(num && HWInterface.AHT21.ConnectionError)
    {
      num--;
      HWInterface.AHT21.ConnectionError = HWInterface.AHT21.Init();
    }

    num = 3;
    while(num && HWInterface.Ecompass.ConnectionError)
    {
      num--;
      HWInterface.Ecompass.ConnectionError = HWInterface.Ecompass.Init();
    }
    if(!HWInterface.Ecompass.ConnectionError)
      HWInterface.Ecompass.Sleep();

    num = 3;
    while(num && HWInterface.Barometer.ConnectionError)
    {
      num--;
      HWInterface.Barometer.ConnectionError = HWInterface.Barometer.Init();
    }

    num = 3;
    while(num && HWInterface.IMU.ConnectionError)
    {
      num--;
      HWInterface.IMU.ConnectionError = HWInterface.IMU.Init();
      // Sensor_MPU_Erro = MPU_Init();
    }

    num = 3;
    while(num && HWInterface.HR_meter.ConnectionError)
    {
      num--;
      HWInterface.HR_meter.ConnectionError = HWInterface.HR_meter.Init();
    }
    if(!HWInterface.HR_meter.ConnectionError)
      HWInterface.HR_meter.Sleep();


    // EEPROM
    EEPROM_Init();
    if(!EEPROM_Check())
    {
      uint8_t recbuf[3];
      SettingGet(recbuf,0x10,2);
      if((recbuf[0]!=0 && recbuf[0]!=1) || (recbuf[1]!=0 && recbuf[1]!=1))
      {
        HWInterface.IMU.wrist_is_enabled = 0;
        ui_APPSy_EN = 0;
      }
      else
      {
        HWInterface.IMU.wrist_is_enabled = recbuf[0];
        ui_APPSy_EN = recbuf[1];
      }

      RTC_DateTypeDef nowdate;
      HAL_RTC_GetDate(&hrtc,&nowdate,RTC_FORMAT_BIN);

      SettingGet(recbuf,0x20,3);
      if(recbuf[0] == nowdate.Date)
      {
        uint16_t steps=0;
        steps = recbuf[1]&0x00ff;
        steps = steps<<8 | recbuf[2];
        if(!HWInterface.IMU.ConnectionError)
          dmp_set_pedometer_step_count((unsigned long)steps);
      }
    }


    // BLE
    HWInterface.BLE.Init();
    HWInterface.BLE.Disable();

    //set the KT6328 BautRate 9600
    //default is 115200
    //printf("AT+CT01\r\n");

    // touch
    CST816_GPIO_Init();
    CST816_RESET();

    // lcd
    LCD_Init();
    LCD_Fill(0,0, LCD_W, LCD_H, BLACK);
    delay_ms(10);
    LCD_Set_Light(50);
    LCD_ShowString(72,LCD_H/2,(uint8_t*)"Welcome!", WHITE, BLACK, 24, 0);//12*6,16*8,24*12,32*16
    uint8_t lcd_buf_str[17];
    sprintf(lcd_buf_str, "OV-Watch V%d.%d.%d", watch_version_major(), watch_version_minor(), watch_version_patch());
    LCD_ShowString(34, LCD_H/2+48, (uint8_t*)lcd_buf_str, WHITE, BLACK, 24, 0);
    delay_ms(1000);
    LCD_Fill(0, LCD_H/2-24, LCD_W, LCD_H/2+49, BLACK);


    // ui
    // LVGL init
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    ui_init();

    xTaskResumeAll();
		vTaskDelete(NULL);
		osDelay(500);
	}
}


