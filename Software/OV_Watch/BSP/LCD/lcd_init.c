#include "lcd_init.h"
#include "delay.h"
#include "spi.h"
#include "tim.h"

/**
  * @brief  初始化 LCD 的控制脚：RES(PB7)、CS(PB8)、DC(PB9)
  * @retval None
  * @note   只配了这三个普通 GPIO；SCK(PB3)、MOSI(PB5) 是 SPI1 的复用脚，
  *         由 CubeMX 生成的 MX_SPI1_Init 配置，这里不管
  *         配完先把三个脚都拉高：片选无效、不复位、DC 处于写数据状态
  */

/******************************************************************************
      函数说明：LCD端口初始化
      入口数据：无
      返回值：  无
******************************************************************************/
void LCD_GPIO_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure = {0};
	
 	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	GPIO_InitStructure.Pin = RES_PIN|CS_PIN|DC_PIN;	 
 	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP; 		 //推挽输出
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;//速度50MHz
 	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);	  //初始化GPIOB
 	HAL_GPIO_WritePin(GPIOB, RES_PIN|CS_PIN|DC_PIN, GPIO_PIN_SET);
}


/**
  * @brief  通过 SPI1 发送一个字节
  * @param  dat 要发送的字节
  * @retval None
  * @note   实际走的是硬件 SPI（HAL_SPI_Transmit），超时 1 ms；
  *         下面被注释掉的那段是软件模拟 SPI，只有在没开硬件 SPI 时才需要
  */

/******************************************************************************
      函数说明：LCD串行数据写入函数(software SPI)
      入口数据：dat  要写入的串行数据
      返回值：  无
******************************************************************************/
void LCD_Writ_Bus(u8 dat) 
{	
	//hard SPI
	HAL_SPI_Transmit(&hspi1,&dat,1,1);
	
	//soft SPI
	/*
	u8 i;
	for(i=0;i<8;i++)
	{			  
		LCD_SCLK_Clr();
		if(dat&0x80)
		{
		   LCD_MOSI_Set();
		}
		else
		{
		   LCD_MOSI_Clr();
		}
		LCD_SCLK_Set();
		dat<<=1;
	}	
	*/
}


/**
  * @brief  写一个字节的数据（DC 保持高电平）
  * @param  dat 数据字节
  * @retval None
  * @note   只是 LCD_Writ_Bus 的别名；此时 DC 为高，屏把它当数据而不是命令
  */

/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA8(u8 dat)
{
	LCD_Writ_Bus(dat);
}


/**
  * @brief  写一个 16 位数据（例如颜色、坐标）
  * @param  dat 16 位数据
  * @retval None
  * @note   拆成高字节、低字节先后发送，符合 ST7789 的大端约定；
  *         直接调用 HAL_SPI_Transmit 发 2 字节，比调两次 LCD_Writ_Bus 省一次函数调用
  */

/******************************************************************************
      函数说明：LCD写入数据
      入口数据：dat 写入的数据
      返回值：  无
******************************************************************************/
void LCD_WR_DATA(u16 dat)
{
//	LCD_Writ_Bus(dat>>8);
//	LCD_Writ_Bus(dat);
	uint8_t temp[2];
	temp[0]=(dat>>8)&0xff;
	temp[1]=dat&0xff;
	HAL_SPI_Transmit(&hspi1,temp,2,1);
	
}


/**
  * @brief  写一条命令
  * @param  dat 命令码
  * @retval None
  * @note   关键在于 DC 脚：拉低表示"接下来发的是命令"，发完立刻拉高，
  *         后续的 LCD_WR_DATA8 就都被当作这条命令的参数
  */

/******************************************************************************
      函数说明：LCD写入命令
      入口数据：dat 写入的命令
      返回值：  无
******************************************************************************/
void LCD_WR_REG(u8 dat)
{
	LCD_DC_Clr();//写命令
	LCD_Writ_Bus(dat);
	LCD_DC_Set();//写数据
}


/**
  * @brief  设置接下来要写入的矩形区域（开窗）
  * @param  x1 y1 左上角坐标
  * @param  x2 y2 右下角坐标
  * @retval None
  * @note   0x2A 写列起止，0x2B 写行起止，0x2C 是"开始写显存"；
  *         调完这个函数以后，连续发的每个 16 位颜色会自动按行填满整个窗口，
  *         填满一行自动换下一行，不用再报坐标
  */

/******************************************************************************
      函数说明：设置起始和结束地址
      入口数据：x1,x2 设置列的起始和结束地址
                y1,y2 设置行的起始和结束地址
      返回值：  无
******************************************************************************/
void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2)
{
	LCD_WR_REG(0x2a);//列地址设置
	LCD_WR_DATA(x1);
	LCD_WR_DATA(x2);
	LCD_WR_REG(0x2b);//行地址设置
	LCD_WR_DATA(y1);
	LCD_WR_DATA(y2);
	LCD_WR_REG(0x2c);//储存器写
}


/**
  * @brief  调节背光亮度
  * @param  dc 占空比，5~100，单位是百分比
  * @retval None
  * @note   背光接在 TIM3_CH3（PB0）上，PWM 周期 PWM_PERIOD=300；
  *         传入 5~100 以外的值时什么都不做，背光保持原亮度
  */

/******************************************************************************
      函数说明：LCD调节背光
      入口数据：dc,占空比,5%~100%
      返回值：  无
******************************************************************************/
void LCD_Set_Light(uint8_t dc)
{
	if(dc>=5 && dc<=100)
		__HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_3,dc*PWM_PERIOD/100);
}


/**
  * @brief  关闭背光：把 PWM 占空比设成 0
  * @retval None
  * @note   只改比较值，定时器仍在跑；下一行被注释掉的才是真正停掉 PWM
  */

/******************************************************************************
      函数说明：LCD关闭背光
      入口数据：无
      返回值：  无
******************************************************************************/
void LCD_Close_Light(void)
{
	__HAL_TIM_SetCompare(&htim3,TIM_CHANNEL_3,0);
	//HAL_TIM_PWM_Stop(&htim3,TIM_CHANNEL_3);
}


/**
  * @brief  启动背光 PWM
  * @retval None
  * @note   只启动定时器，亮度用的是上一次 LCD_Set_Light 设的占空比；
  *         如果从没设过，占空比是 0，屏幕看起来还是黑的
  */

/******************************************************************************
      函数说明：LCD开启背光
      入口数据：无
      返回值：  无
******************************************************************************/
void LCD_Open_Light(void)
{
	HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_3);
}

/**
  * @brief  让 ST7789 进入睡眠
  * @retval None
  * @note   命令 0x10，之后要等 100 ms 屏才真正睡下；睡眠时显存内容会丢
  */

/******************************************************************************
      函数说明：ST7789 SLEEP IN
      入口数据：无
      返回值：  无
******************************************************************************/
void LCD_ST7789_SleepIn(void)
{
	LCD_WR_REG(0x10);
	delay_ms(100);
}


/**
  * @brief  唤醒 ST7789
  * @retval None
  * @note   命令 0x11，要等 100 ms 屏才恢复；醒来后需要重新刷一次画面
  */

/******************************************************************************
      函数说明：ST7789 SLEEP OUT
      入口数据：无
      返回值：  无
******************************************************************************/
void LCD_ST7789_SleepOut(void)
{
	LCD_WR_REG(0x11);
	delay_ms(100);
}


/**
  * @brief  ST7789 的完整上电初始化
  * @retval None
  * @note   顺序：配 GPIO -> 片选拉低 -> 硬复位（RES 低 100 ms 再高）
  *         -> 0x11 退出睡眠 -> 0x36 设扫描方向 -> 0x3A 设 16 位色
  *         -> 电源/门限/伽马等一长串厂商推荐值 -> 0x21 反显 -> 0x29 开显示
  *         0x36 的取值由 USE_HORIZONTAL 决定，0/1 竖屏，2/3 横屏
  *         0x21 是反显开关：这颗 IPS 面板本身是反相的，打开它颜色才正常
  */

/******************************************************************************
      函数说明：LCD初始化
      入口数据：无
      返回值：  无
******************************************************************************/
void LCD_Init(void)
{
	LCD_GPIO_Init();//初始化GPIO
	LCD_CS_Clr();		//chip select
	
	LCD_RES_Clr();	//复位
	delay_ms(100);
	LCD_RES_Set();
	delay_ms(100);
	
	LCD_WR_REG(0x11); 
	delay_ms(120); 
	LCD_WR_REG(0x36); 
	if(USE_HORIZONTAL==0)LCD_WR_DATA8(0x00);
	else if(USE_HORIZONTAL==1)LCD_WR_DATA8(0xC0);
	else if(USE_HORIZONTAL==2)LCD_WR_DATA8(0x70);
	else LCD_WR_DATA8(0xA0);

	LCD_WR_REG(0x3A);
	LCD_WR_DATA8(0x05);

	LCD_WR_REG(0xB2);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x00);
	LCD_WR_DATA8(0x33);
	LCD_WR_DATA8(0x33); 

	LCD_WR_REG(0xB7); 
	LCD_WR_DATA8(0x35);  

	LCD_WR_REG(0xBB);
	LCD_WR_DATA8(0x19);

	LCD_WR_REG(0xC0);
	LCD_WR_DATA8(0x2C);

	LCD_WR_REG(0xC2);
	LCD_WR_DATA8(0x01);

	LCD_WR_REG(0xC3);
	LCD_WR_DATA8(0x12);   

	LCD_WR_REG(0xC4);
	LCD_WR_DATA8(0x20);  

	LCD_WR_REG(0xC6); 
	LCD_WR_DATA8(0x0F);    

	LCD_WR_REG(0xD0); 
	LCD_WR_DATA8(0xA4);
	LCD_WR_DATA8(0xA1);

	LCD_WR_REG(0xE0);
	LCD_WR_DATA8(0xD0);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x0D);
	LCD_WR_DATA8(0x11);
	LCD_WR_DATA8(0x13);
	LCD_WR_DATA8(0x2B);
	LCD_WR_DATA8(0x3F);
	LCD_WR_DATA8(0x54);
	LCD_WR_DATA8(0x4C);
	LCD_WR_DATA8(0x18);
	LCD_WR_DATA8(0x0D);
	LCD_WR_DATA8(0x0B);
	LCD_WR_DATA8(0x1F);
	LCD_WR_DATA8(0x23);

	LCD_WR_REG(0xE1);
	LCD_WR_DATA8(0xD0);
	LCD_WR_DATA8(0x04);
	LCD_WR_DATA8(0x0C);
	LCD_WR_DATA8(0x11);
	LCD_WR_DATA8(0x13);
	LCD_WR_DATA8(0x2C);
	LCD_WR_DATA8(0x3F);
	LCD_WR_DATA8(0x44);
	LCD_WR_DATA8(0x51);
	LCD_WR_DATA8(0x2F);
	LCD_WR_DATA8(0x1F);
	LCD_WR_DATA8(0x1F);
	LCD_WR_DATA8(0x20);
	LCD_WR_DATA8(0x23);

	LCD_WR_REG(0x21); 

	LCD_WR_REG(0x29); 
}



